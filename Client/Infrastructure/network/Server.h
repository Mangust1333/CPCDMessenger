#pragma once

#include "ClientSession.h"
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

class Server : public IServer {
public:
    Server(boost::asio::io_context& ioc, unsigned short port)
            : acceptor_(ioc, tcp::endpoint(tcp::v4(), port)),
              ioc_(ioc)
    {
        do_accept();
    }

    void register_username(const std::string& user, std::shared_ptr<ISession> session) override;
    void unregister_username(const std::string& user) override;
    void route_message(const std::string& to, const json& message) override;

private:
    void do_accept();

    tcp::acceptor acceptor_;
    boost::asio::io_context& ioc_;

    std::unordered_map<std::string, std::weak_ptr<ISession>> clients_;
    std::mutex clients_mutex_;

    std::unordered_map<std::string, std::vector<std::string>> offline_messages_;
    std::mutex offline_mutex_;
};

