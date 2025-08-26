#pragma once

#include "ISession.h"
#include "IServer.h"

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

class ClientSession
        : public std::enable_shared_from_this<ClientSession>,
          public ISession
{
public:
    ClientSession(tcp::socket socket, IServer& server)
            : socket_(std::move(socket)),
              server_(server),
              strand_(socket_.get_executor()),
              closed_(false)
    {}

    void start();
    void deliver_json(const json& j) override;
    std::string username() const override;
    void close() override;

private:
    void do_read();
    void on_read(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void do_write();
    void on_write(const boost::system::error_code& ec, std::size_t bytes_transferred);

    tcp::socket socket_;
    IServer& server_;
    boost::asio::strand<boost::asio::any_io_executor> strand_;
    boost::asio::streambuf read_buf_;

    std::deque<std::string> write_msgs_;
    mutable std::mutex mutex_;
    std::string username_;
    std::atomic<bool> writing_{false};
    std::atomic<bool> closed_;
};
