#include <functional>
#include <argument_parser.h>
#include <text_parser_lib.h>

#include <iostream>
#include <fstream>
#include <numeric>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socket_t = SOCKET;
#define CLOSESOCK closesocket
#else
#include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  using socket_t = int;
  #define CLOSESOCK close
#endif

#include <cstring>
#include <stdexcept>
#include <unistd.h>

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

void runSender(const std::string& inputPath, const std::string& host) {
    socket_t sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) throw std::runtime_error("Socket creation failed");

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address: " + host);
    }

    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        throw std::runtime_error("Connection failed");
    }

    FileParser::FileBlockIterator begin(inputPath, BLOCK_SIZE);
    FileParser::FileBlockIterator end(inputPath, BLOCK_SIZE, true);

    while (begin != end) {
        size_t blockSize = begin->size();
        if (send(sockfd, reinterpret_cast<const char*>(&blockSize), sizeof(blockSize), 0) <= 0) break;
        if (send(sockfd, begin->data(), (int)blockSize, 0) <= 0) break;
        ++begin;
    }

    size_t zero = 0;
    send(sockfd, reinterpret_cast<const char*>(&zero), sizeof(zero), 0);

    CLOSESOCK(sockfd);
    std::cout << "Файл " << inputPath << " успешно отправлен." << std::endl;
}

void runReceiver(const std::string& outputPath) {
    socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) throw std::runtime_error("Socket creation failed");

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Bind failed");
    }

    if (listen(server_fd, 1) < 0) {
        throw std::runtime_error("Listen failed");
    }

    std::cout << "Ожидание подключения..." << std::endl;
    socklen_t addrlen = sizeof(address);
    socket_t sock = accept(server_fd, (sockaddr*)&address, &addrlen);
    if (sock < 0) throw std::runtime_error("Accept failed");

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) throw std::runtime_error("Failed to open output file: " + outputPath);

    while (true) {
        size_t blockSize;
        int bytes = recv(sock, reinterpret_cast<char*>(&blockSize), sizeof(blockSize), MSG_WAITALL);
        if (bytes <= 0 || blockSize == 0) break;

        std::vector<char> buffer(blockSize);
        int received = recv(sock, buffer.data(), (int)blockSize, MSG_WAITALL);
        if (received <= 0) break;

        outFile.write(buffer.data(), received);
    }

    CLOSESOCK(sock);
    CLOSESOCK(server_fd);
    outFile.close();

    std::cout << "Файл успешно сохранён в " << outputPath << std::endl;
}

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);

    std::string inputPath;
    std::string outputPath;
    std::string mode;
    std::string host = "127.0.0.1";

    ArgumentParser::ArgParser parser("FileTransfer");

    parser.AddStringArgument('m', "mode", "sender or receiver").StoreValue(mode);
    parser.AddStringArgument('p', "path", "path to input/output file").StoreValue(inputPath);
    parser.AddStringArgument('o', "output", "output file path (receiver only)").StoreValue(outputPath);
    parser.AddStringArgument('H', "host", "server host (sender only)").StoreValue(host);
    parser.AddHelp('h', "help", "File transfer utility");

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
        if (mode == "sender") {
            if (inputPath.empty()) throw std::runtime_error("Missing input path");
            runSender(inputPath, host);
        } else if (mode == "receiver") {
            if (outputPath.empty()) throw std::runtime_error("Missing output path");
            runReceiver(outputPath);
        } else {
            throw std::runtime_error("Mode must be 'sender' or 'receiver'");
        }
        cleanupSockets();
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl;
        cleanupSockets();
        return 1;
    }

    return 0;
}
