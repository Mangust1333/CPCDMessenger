#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <string>
#include "../Infrastructure/network/ClientSession.h"
#include "../Infrastructure/network/ConsoleClient.h"
#include "../Infrastructure/network/Server.h"
#include "argument_parser.h"
#include "nlohmann/json.hpp"

using boost::asio::ip::tcp;
using json = nlohmann::json;

void run_server(unsigned short port) {
    try {
        boost::asio::io_context ioc{1};
        Server server(ioc, port);
        std::cout << "Relay server running on port " << port << "\n";

        unsigned int nthreads = std::max(1u, std::thread::hardware_concurrency());
        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < nthreads - 1; ++i) {
            threads.emplace_back([&ioc]{ ioc.run(); });
        }
        ioc.run();
        for (auto &t : threads) t.join();
    } catch (std::exception& ex) {
        std::cerr << "Server fatal: " << ex.what() << "\n";
    }
}

void run_client(const std::string& host, unsigned short port) {
    boost::asio::io_context ioc;
    auto client = std::make_shared<ConsoleClient>(ioc, host, port);
    client->start();

    // Запускаем ioc в отдельном потоке
    std::thread ioc_thread([&ioc]{ ioc.run(); });

    std::cout << "Console client. Команды:\n";
    std::cout << "  /login <username>\n";
    std::cout << "  /msg <to> <message>\n";
    std::cout << "  /quit\n";
    std::cout << "Чтобы отправить сообщение без команды, используйте: /msg <to> <message>\n";

    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        size_t first_non = line.find_first_not_of(" \t");
        if (first_non == std::string::npos) continue;
        if (first_non > 0) line = line.substr(first_non);

        if (line.rfind("/login ", 0) == 0) {
            std::string user = line.substr(7);
            client->send_login(user);
        } else if (line.rfind("/msg ", 0) == 0) {
            std::string rest = line.substr(5);
            std::istringstream iss(rest);
            std::string to;
            if (!(iss >> to)) {
                std::cout << "Usage: /msg <to> <message>\n";
                continue;
            }
            std::string body;
            std::getline(iss, body);
            if (!body.empty() && body[0] == ' ') body.erase(0,1);
            client->send_message(to, body);
        } else if (line == "/quit") {
            break;
        } else if (line == "/help") {
            std::cout << "Команды: /login /msg /quit /help\n";
        } else {
            std::cout << "Неизвестная команда. Введите /help.\n";
        }
    }

    client->stop();
    ioc.stop();
    if (ioc_thread.joinable()) ioc_thread.join();
}

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::string mode = "server";
    std::string host = "127.0.0.1";
    int port_int = 5555;

    ArgumentParser::ArgParser parser("MessengerRelay");
    parser.AddStringArgument('m', "mode", "server | client").StoreValue(mode).Default("server");
    parser.AddStringArgument('H', "host", "relay host").StoreValue(host).Default("127.0.0.1");
    parser.AddIntArgument('P', "port", "relay port").StoreValue(port_int).Default(5555);
    parser.AddHelp('h', "help", "Messenger with relay server");

    if (!parser.Parse(argc, argv)) {
        std::cerr << "Invalid arguments\n" << parser.HelpDescription() << std::endl;
        return 1;
    }
    if (parser.Help()) {
        std::cout << parser.HelpDescription() << std::endl;
        return 0;
    }

    if (port_int < 0 || port_int > 65535) {
        std::cerr << "Invalid port: " << port_int << std::endl;
        return 1;
    }
    unsigned short port = static_cast<unsigned short>(port_int);

    try {
        if (mode == "server" || mode == "relay") {
            run_server(port);
        } else if (mode == "client") {
            run_client(host, port);
        } else {
            std::cerr << "Unknown mode: " << mode << "\n";
            std::cerr << parser.HelpDescription() << std::endl;
            return 1;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}