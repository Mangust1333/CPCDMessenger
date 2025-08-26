#include "Server.h"

void Server::register_username(const std::string& user, std::shared_ptr<ISession> session) {
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

void Server::unregister_username(const std::string &user) {
    std::lock_guard<std::mutex> lk(clients_mutex_);
    auto it = clients_.find(user);
    if (it != clients_.end()) {
        clients_.erase(it);
    }
}

void Server::route_message(const std::string &to, const json &message) {
    std::shared_ptr<ISession> dest;
    {
        std::lock_guard<std::mutex> lk(clients_mutex_);
        auto it = clients_.find(to);
        if (it != clients_.end())
            dest = it->second.lock();
    }

    if (dest) {
        dest->deliver_json(message);
    } else {
        std::lock_guard<std::mutex> lk(offline_mutex_);
        offline_messages_[to].push_back(message.dump());
    }
}

void Server::do_accept() {
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
