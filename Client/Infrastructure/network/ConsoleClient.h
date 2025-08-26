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
#include "nlohmann/json.hpp"

using boost::asio::ip::tcp;
using json = nlohmann::json;

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

    void start();
    void stop();
    void send_login(const std::string& user);
    void send_message(const std::string& to, const std::string& body);

private:
    void do_read();
    void handle_server_json(const json& j);
    void send_json(const json& j);
    void do_write();

    tcp::socket socket_;
    tcp::resolver resolver_;
    boost::asio::strand<boost::asio::any_io_executor> strand_;
    boost::asio::streambuf read_buf_;
    std::deque<std::string> write_msgs_;
    std::string host_;
    unsigned short port_;
    std::atomic<bool> stopped_;
};