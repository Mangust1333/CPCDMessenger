#include "ClientSession.h"

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

void ClientSession::on_write(const boost::system::error_code& ec, std::size_t) {
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

std::string ClientSession::username() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return username_;
}
