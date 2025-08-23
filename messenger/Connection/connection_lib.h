#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <deque>
#include <vector>
#include <string>
#include <atomic>
#include <optional>
#include <thread>
#include <nlohmann/json.hpp>

using boost::asio::ip::tcp;
using json = nlohmann::json;

class Server;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(tcp::socket socket, Server& server)
    : socket_(std::move(socket)),
      server_(server),
      strand_(socket_.get_executor()),
      closed_(false)
    {}

    void start();
    void deliver_json(const json& j);
    std::string username() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return username_;
    }
    void close();

private:
    void do_read();
    void on_read(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void do_write();
    void on_write(const boost::system::error_code& ec, std::size_t bytes_transferred);

    tcp::socket socket_;
    Server& server_;
    boost::asio::strand<boost::asio::any_io_executor> strand_;
    boost::asio::streambuf read_buf_;

    std::deque<std::string> write_msgs_;
    mutable std::mutex mutex_;
    std::string username_;
    std::atomic<bool> writing_{false};
    std::atomic<bool> closed_;
};

class Server {
public:
    Server(boost::asio::io_context& ioc, unsigned short port)
    : acceptor_(ioc, tcp::endpoint(tcp::v4(), port)),
      ioc_(ioc)
    {
        do_accept();
    }

    void register_username(const std::string& user, std::shared_ptr<ClientSession> session) {
        std::lock_guard<std::mutex> lk(clients_mutex_);
        clients_[user] = session;

        auto it = offline_messages_.find(user);
        if (it != offline_messages_.end()) {
            for (auto &m : it->second) {
                session->deliver_json(json::parse(m));
            }
            offline_messages_.erase(it);
        }
    }

    void unregister_username(const std::string& user) {
        std::lock_guard<std::mutex> lk(clients_mutex_);
        auto it = clients_.find(user);
        if (it != clients_.end()) {
            clients_.erase(it);
        }
    }

    void route_message(const std::string& to, const json& message) {
        std::shared_ptr<ClientSession> dest;
        {
            std::lock_guard<std::mutex> lk(clients_mutex_);
            auto it = clients_.find(to);
            if (it != clients_.end()) dest = it->second.lock();
        }

        if (dest) {
            dest->deliver_json(message);
        } else {
            std::lock_guard<std::mutex> lk(offline_mutex_);
            offline_messages_[to].push_back(message.dump());
        }
    }

private:
    void do_accept() {
        acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto session = std::make_shared<ClientSession>(std::move(socket), *this);
                session->start();
            } else {
                std::cerr << "Accept error: " << ec.message() << "\n";
            }
            do_accept();
        });
    }

    tcp::acceptor acceptor_;
    boost::asio::io_context& ioc_;

    std::unordered_map<std::string, std::weak_ptr<ClientSession>> clients_;
    std::mutex clients_mutex_;

    std::unordered_map<std::string, std::vector<std::string>> offline_messages_;
    std::mutex offline_mutex_;
};

void ClientSession::start() {
    do_read();
}

void ClientSession::do_read() {
    auto self = shared_from_this();
    boost::asio::async_read_until(socket_, read_buf_, '\n',
        boost::asio::bind_executor(strand_,
            [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                on_read(ec, bytes_transferred);
            }
        ));
}

void ClientSession::on_read(const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if (ec) {
        std::string usr;
        {
            std::lock_guard<std::mutex> lk(mutex_);
            usr = username_;
        }
        if (!usr.empty()) server_.unregister_username(usr);
        close();
        return;
    }

    std::istream is(&read_buf_);
    std::string line;
    std::getline(is, line);
    if (!line.empty() && line.back() == '\r') line.pop_back();

    try {
        auto j = json::parse(line);
        if (j.contains("cmd")) {
            std::string cmd = j["cmd"].get<std::string>();
            if (cmd == "login" && j.contains("user")) {
                std::string user = j["user"].get<std::string>();
                {
                    std::lock_guard<std::mutex> lk(mutex_);
                    username_ = user;
                }
                server_.register_username(user, shared_from_this());
                json resp = { {"type","login_ok"}, {"user", user} };
                deliver_json(resp);
            } else if (cmd == "msg") {
                if (j.contains("to") && j.contains("body")) {
                    std::string to = j["to"].get<std::string>();
                    std::string body = j["body"].get<std::string>();
                    std::string from = username();
                    json out = { {"type","msg"}, {"from", from}, {"body", body} };
                    server_.route_message(to, out);
                } else {
                    json resp = { {"type","error"}, {"message","invalid msg format"} };
                    deliver_json(resp);
                }
            } else {
                json resp = { {"type","error"}, {"message","unknown cmd"} };
                deliver_json(resp);
            }
        } else {
            json resp = { {"type","error"}, {"message","no cmd field"} };
            deliver_json(resp);
        }
    } catch (std::exception& ex) {
        json resp = { {"type","error"}, {"message", std::string("json parse error: ") + ex.what()} };
        deliver_json(resp);
    }

    do_read();
}

void ClientSession::deliver_json(const json& j) {
    std::string s = j.dump();
    s.push_back('\n');

    auto self = shared_from_this();
    boost::asio::post(strand_, [this, self, s = std::move(s)]() mutable {
        bool start_write = write_msgs_.empty();
        write_msgs_.push_back(std::move(s));
        if (start_write && !writing_) {
            do_write();
        }
    });
}

void ClientSession::do_write() {
    if (closed_) return;
    if (write_msgs_.empty()) {
        writing_ = false;
        return;
    }
    writing_ = true;
    auto self = shared_from_this();
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().size()),
        boost::asio::bind_executor(strand_,
            [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                on_write(ec, bytes_transferred);
            }
        ));
}

void ClientSession::on_write(const boost::system::error_code& ec, std::size_t /*bytes_transferred*/) {
    if (ec) {
        std::string usr;
        {
            std::lock_guard<std::mutex> lk(mutex_);
            usr = username_;
        }
        if (!usr.empty()) server_.unregister_username(usr);
        close();
        return;
    }
    write_msgs_.pop_front();
    if (!write_msgs_.empty()) {
        do_write();
    } else {
        writing_ = false;
    }
}

void ClientSession::close() {
    if (closed_.exchange(true)) return;
    boost::system::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_both, ec);
    socket_.close(ec);
}

class ConsoleClient : public std::enable_shared_from_this<ConsoleClient> {
public:
    ConsoleClient(boost::asio::io_context& ioc, const std::string& host, unsigned short port)
            : socket_(ioc),
              resolver_(ioc),
              strand_(ioc.get_executor()),
              host_(host),
              port_(port),
              stopped_(false)
    {}

    void start() {
        auto endpoints = resolver_.resolve(host_, std::to_string(port_));
        boost::asio::async_connect(socket_, endpoints,
                                   boost::asio::bind_executor(strand_, [self=shared_from_this()](const boost::system::error_code& ec, const tcp::endpoint&) {
                                       if (ec) {
                                           std::cerr << "Connect failed: " << ec.message() << "\n";
                                           self->stop();
                                           return;
                                       }
                                       self->do_read();
                                   })
        );
    }

    void stop() {
        if (stopped_.exchange(true)) return;
        boost::system::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_both, ec);
        socket_.close(ec);
    }

    void send_login(const std::string& user) {
        json j = { {"cmd", "login"}, {"user", user} };
        send_json(j);
    }

    void send_message(const std::string& to, const std::string& body) {
        json j = { {"cmd","msg"}, {"to", to}, {"body", body} };
        send_json(j);
    }

private:
    void do_read() {
        auto self = shared_from_this();
        boost::asio::async_read_until(socket_, read_buf_, '\n',
                                      boost::asio::bind_executor(strand_, [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred){
                                          if (ec) {
                                              if (!stopped_) std::cerr << "Connection closed: " << ec.message() << "\n";
                                              stop();
                                              return;
                                          }
                                          std::istream is(&read_buf_);
                                          std::string line;
                                          std::getline(is, line);
                                          if (!line.empty() && line.back() == '\r') line.pop_back();

                                          try {
                                              auto j = json::parse(line);
                                              handle_server_json(j);
                                          } catch (std::exception& ex) {
                                              std::cerr << "Received invalid json: " << ex.what() << "\n";
                                          }

                                          do_read();
                                      })
        );
    }

    void handle_server_json(const json& j) {
        if (j.contains("type")) {
            std::string t = j["type"].get<std::string>();
            if (t == "msg") {
                std::string from = j.value("from", "");
                std::string body = j.value("body", "");
                std::cout << "\n[" << from << "] " << body << "\n> " << std::flush;
            } else if (t == "login_ok") {
                std::string user = j.value("user", "");
                std::cout << "\n[system] logged in as " << user << "\n> " << std::flush;
            } else if (t == "error") {
                std::string msg = j.value("message", "");
                std::cout << "\n[server error] " << msg << "\n> " << std::flush;
            } else {
                std::cout << "\n[server] " << j.dump() << "\n> " << std::flush;
            }
        } else {
            std::cout << "\n[server raw] " << j.dump() << "\n> " << std::flush;
        }
    }

    void send_json(const json& j) {
        std::string s = j.dump();
        s.push_back('\n');

        boost::asio::post(strand_, [this, self=shared_from_this(), s = std::move(s)]() mutable {
            bool write_in_progress = !write_msgs_.empty();
            write_msgs_.push_back(std::move(s));
            if (!write_in_progress) do_write();
        });
    }

    void do_write() {
        auto self = shared_from_this();
        boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front()),
                                 boost::asio::bind_executor(strand_, [this, self](const boost::system::error_code& ec, std::size_t /*bytes_transferred*/){
                                     if (ec) {
                                         std::cerr << "Write error: " << ec.message() << "\n";
                                         stop();
                                         return;
                                     }
                                     write_msgs_.pop_front();
                                     if (!write_msgs_.empty()) do_write();
                                 })
        );
    }

    tcp::socket socket_;
    tcp::resolver resolver_;
    boost::asio::strand<boost::asio::any_io_executor> strand_;
    boost::asio::streambuf read_buf_;
    std::deque<std::string> write_msgs_;
    std::string host_;
    unsigned short port_;
    std::atomic<bool> stopped_;
};
