#include "ConsoleClient.h"

void ConsoleClient::start() {
    auto endpoints = resolver_.resolve(host_, std::to_string(port_));
    boost::asio::async_connect(socket_, endpoints,
       boost::asio::bind_executor(strand_,
        [self=shared_from_this()](const boost::system::error_code& ec, const tcp::endpoint&)
        {
            if (ec) {
               std::cerr << "Connect failed: " << ec.message() << "\n";
               self->stop();
               return;
            }
            self->do_read();
       })
    );
}

void ConsoleClient::stop() {
    if (stopped_.exchange(true)) return;
    boost::system::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_both, ec);
    socket_.close(ec);
}

void ConsoleClient::send_login(const std::string& user) {
    json j = { {"cmd", "login"}, {"user", user} };
    send_json(j);
}

void ConsoleClient::send_message(const std::string& to, const std::string& body) {
    json j = { {"cmd","msg"}, {"to", to}, {"body", body} };
    send_json(j);
}

void ConsoleClient::do_read() {
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

void ConsoleClient::handle_server_json(const json &j) {
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

void ConsoleClient::send_json(const json &j) {
    std::string s = j.dump();
    s.push_back('\n');

    boost::asio::post(strand_, [this, self=shared_from_this(), s = std::move(s)]() mutable {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(std::move(s));
        if (!write_in_progress) do_write();
    });
}

void ConsoleClient::do_write() {
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
