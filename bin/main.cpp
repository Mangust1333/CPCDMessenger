#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <stdexcept>
#include <cstring>

#include <argument_parser.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include "text_parser_lib.h"

#pragma comment(lib, "ws2_32.lib")
using socket_t = SOCKET;
#define CLOSESOCK closesocket
#else
#include <arpa/inet.h>
#include <unistd.h>
using socket_t = int;
#define CLOSESOCK close
#endif

constexpr int PORT = 5000;
constexpr int BLOCK_SIZE = 1024;

void initSockets() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif
}

void cleanupSockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}


void forward(socket_t from, socket_t to) {
    char buffer[BLOCK_SIZE];
    while (true) {
        int n = recv(from, buffer, sizeof(buffer), 0);
        if (n <= 0) break;
        send(to, buffer, n, 0);
    }
    CLOSESOCK(from);
    CLOSESOCK(to);
}

void runRelay() {
    socket_t server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) throw std::runtime_error("socket failed");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind failed");
    if (listen(server, 2) < 0)
        throw std::runtime_error("listen failed");

    std::cout << "Relay server listening on port " << PORT << "...\n";


    sockaddr_in caddr{};
    socklen_t clen = sizeof(caddr);
    socket_t receiver = accept(server, (sockaddr*)&caddr, &clen);
    if (receiver < 0) throw std::runtime_error("accept receiver failed");
    std::cout << "Receiver connected\n";


    socket_t sender = accept(server, (sockaddr*)&caddr, &clen);
    if (sender < 0) throw std::runtime_error("accept sender failed");
    std::cout << "Sender connected\n";


    std::thread t1(forward, sender, receiver);
    std::thread t2(forward, receiver, sender);
    t1.join();
    t2.join();

    CLOSESOCK(server);
    std::cout << "Relay finished\n";
}


void runReceiver(const std::string& outputPath, const std::string& relayHost) {
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) throw std::runtime_error("socket failed");

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);
    inet_pton(AF_INET, relayHost.c_str(), &serv.sin_addr);

    if (connect(sock, (sockaddr*)&serv, sizeof(serv)) < 0)
        throw std::runtime_error("connect failed (receiver)");

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) throw std::runtime_error("open output file failed");

    char buffer[BLOCK_SIZE];
    while (true) {
        int n = recv(sock, buffer, sizeof(buffer), 0);
        if (n <= 0) break;
        outFile.write(buffer, n);
    }

    CLOSESOCK(sock);
    outFile.close();
    std::cout << "Файл сохранён в " << outputPath << "\n";
}


void runSender(const std::string& inputPath, const std::string& relayHost) {
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) throw std::runtime_error("socket failed");

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);
    inet_pton(AF_INET, relayHost.c_str(), &serv.sin_addr);

    if (connect(sock, (sockaddr*)&serv, sizeof(serv)) < 0)
        throw std::runtime_error("connect failed (sender)");

    std::ifstream inFile(inputPath, std::ios::binary);
    if (!inFile) throw std::runtime_error("Failed to open input file: " + inputPath);

    FileParser::FileBlockIterator begin(inputPath, BLOCK_SIZE);
    FileParser::FileBlockIterator end(inputPath, BLOCK_SIZE, true);

    while (begin != end) {
        send(sock, begin->data(), (int)begin->size(), 0);
        ++begin;
    }

    CLOSESOCK(sock);
    inFile.close();
    std::cout << "Файл отправлен\n";
}


int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);

    std::string mode;
    std::string inputPath;
    std::string outputPath;
    std::string relayHost;

    ArgumentParser::ArgParser parser("FileTransferRelay");
    parser.AddStringArgument('m', "mode", "relay | sender | receiver").StoreValue(mode).Default("relay");
    parser.AddStringArgument('p', "path", "path to input file (sender only)").StoreValue(inputPath);
    parser.AddStringArgument('o', "output", "path to output file (receiver only)").StoreValue(outputPath);
    parser.AddStringArgument('H', "host", "relay host (for sender/receiver)").StoreValue(relayHost).Default("127.0.0.1");
    parser.AddHelp('h', "help", "File transfer utility with relay server");

    if (!parser.Parse(argc, argv)) {
        std::cerr << "Invalid arguments\n" << parser.HelpDescription() << std::endl;
        return 1;
    }
    if (parser.Help()) {
        std::cout << parser.HelpDescription() << std::endl;
        return 0;
    }

    try {
        initSockets();

        if (mode == "relay") {
            runRelay();
        } else if (mode == "receiver") {
            if (outputPath.empty()) throw std::runtime_error("Missing output path");
            runReceiver(outputPath, relayHost);
        } else if (mode == "sender") {
            if (inputPath.empty()) throw std::runtime_error("Missing input path");
            runSender(inputPath, relayHost);
        } else {
            throw std::runtime_error("Unknown mode: " + mode);
        }

        cleanupSockets();
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl;
        cleanupSockets();
        return 1;
    }

    return 0;
}
