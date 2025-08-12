#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/stat.h>
    #include <unistd.h>
#endif

namespace FileParser {
    class FileIdentity {
#ifdef _WIN32
    public:
        explicit FileIdentity(const std::string& path) {
            HANDLE file = CreateFileA(
                    path.c_str(),
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    nullptr
            );

            if (file == INVALID_HANDLE_VALUE) {
                throw std::runtime_error("CreateFileA failed for: " + path);
            }

            BY_HANDLE_FILE_INFORMATION info;
            if (!GetFileInformationByHandle(file, &info)) {
                CloseHandle(file);
                throw std::runtime_error("GetFileInformationByHandle failed for: " + path);
            }

            volumeSerialNumber = info.dwVolumeSerialNumber;
            fileIndexHigh = info.nFileIndexHigh;
            fileIndexLow = info.nFileIndexLow;

            CloseHandle(file);
        }

        bool operator==(const FileIdentity& other) const {
            return volumeSerialNumber == other.volumeSerialNumber &&
                   fileIndexHigh == other.fileIndexHigh &&
                   fileIndexLow == other.fileIndexLow;
        }

        bool operator!=(const FileIdentity& other) const {
            return !(*this == other);
        }

    private:
        DWORD volumeSerialNumber;
        DWORD fileIndexHigh;
        DWORD fileIndexLow;

#else
    public:
        FileIdentity(const std::string& path) {
        struct stat s;
        if (stat(path.c_str(), &s) != 0) {
            throw std::runtime_error("stat() failed for file: " + path);
        }
        dev = s.st_dev;
        ino = s.st_ino;
    }

    bool operator==(const FileIdentity& other) const {
        return dev == other.dev && ino == other.ino;
    }

    private:
        dev_t dev;
        ino_t ino;
#endif
    };

    class FileBlockIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = std::vector<char>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        FileBlockIterator(const std::string& path, std::streamsize blockSize, bool is_end = false)
                : FileBlockIterator(path.c_str(), blockSize, is_end)
        {}

        FileBlockIterator(const char* path, std::streamsize blockSize, bool is_end = false)
        : path_(path),
          fileID_(path),
          in_(path, std::ios::binary),
          blockSize_(blockSize),
          position_(0),
          isEnd_(is_end)
        {
            if (!in_) {
                throw std::runtime_error("Failed to open file");
            }
            ++(*this);
        }

        ~FileBlockIterator() {
            if (in_.is_open()) {
                in_.close();
            }
        }

        FileBlockIterator(const FileBlockIterator& other)
        : path_(other.path_),
          fileID_(other.fileID_),
          blockSize_(other.blockSize_),
          buffer_(other.buffer_),
          position_(other.position_),
          isEnd_(other.isEnd_)
        {
            in_.open(path_, std::ios::binary);
            if (!in_) {
                throw std::runtime_error("Failed to open file in copy assignment");
            }

            if(!isEnd_ && position_ != -1) {
                in_.seekg(position_);
                if (in_.fail()) {
                    throw std::runtime_error("Failed to seek to position");
                }
            }

            verifyFileUnchanged();
        }

        FileBlockIterator& operator=(const FileBlockIterator& other) {
            if(&other == this) {
                return *this;
            }

            if (in_.is_open()) {
                in_.close();
            }

            path_ = other.path_;
            fileID_ = other.fileID_;
            buffer_ = other.buffer_;
            blockSize_ = other.blockSize_;
            position_ = other.position_;
            isEnd_ = other.isEnd_;

            in_.open(path_, std::ios::binary);
            if (!in_) {
                throw std::runtime_error("Failed to open file in copy assignment");
            }

            if(!isEnd_ && position_ != -1) {
                in_.seekg(position_);
                if (in_.fail()) {
                    throw std::runtime_error("Failed to seek to position");
                }
            }

            verifyFileUnchanged();

            return *this;
        }

        FileBlockIterator(FileBlockIterator&& other) noexcept
        : path_(std::move(other.path_)),
          fileID_(std::move(other.fileID_)),
          in_(std::move(other.in_)),
          buffer_(std::move(other.buffer_)),
          blockSize_(other.blockSize_),
          position_(other.position_),
          isEnd_(other.isEnd_)
        {
        }

        FileBlockIterator& operator=(FileBlockIterator&& other) noexcept {
            if(this == &other) {
                return *this;
            }

            path_ = std::move(other.path_);
            fileID_ = std::move(other.fileID_);
            in_ = std::move(other.in_);
            buffer_ = std::move(other.buffer_);
            blockSize_ = other.blockSize_;
            position_ = other.position_;
            isEnd_ = other.isEnd_;

            return *this;
        }

        FileBlockIterator operator++(int) {
            FileBlockIterator tmp = *this;
            readBlock();
            return tmp;
        }

        FileBlockIterator& operator++() {
            readBlock();
            return *this;
        }

        reference operator*() const { return buffer_; }

        pointer operator->() const { return &buffer_; }

        bool operator==(const FileBlockIterator& other) const {
            return fileID_ == other.fileID_ &&
            (position_  == other.position_ && blockSize_ == other.blockSize_ && isEnd_ == other.isEnd_
            || isEnd_ && other.isEnd_);
        }

        bool operator!=(const FileBlockIterator& other) const {
            return !(*this == other);
        }


    private:
        void verifyFileUnchanged() const {
            if (FileIdentity(path_) != fileID_)
                throw std::runtime_error("Underlying file has changed during iteration: " + path_);
        }

        void readBlock() {
            if (isEnd_) return;

            buffer_.assign(blockSize_, '\0');
            in_.read(buffer_.data(), blockSize_);
            std::streamsize n = in_.gcount();

            if (n == 0) {
                isEnd_ = true;
                buffer_.clear();
            } else {
                buffer_.resize(n);
                position_ = in_.tellg();
            }
        }

        std::string         path_;
        FileIdentity        fileID_;
        std::ifstream       in_;
        std::vector<char>   buffer_;
        std::streamsize     blockSize_;
        std::streampos      position_;
        bool                isEnd_;
    };
}
