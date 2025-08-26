#pragma once
#include <string>
#include <chrono>

struct FileTransfer {
    std::string id;
    std::string senderId;
    std::string receiverId;
    std::string fileName;
    size_t fileSize;

    enum class Status {
        Pending,
        InProgress,
        Completed,
        Failed
    } status = Status::Pending;

    std::chrono::system_clock::time_point createdAt;
};
