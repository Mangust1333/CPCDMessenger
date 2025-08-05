#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <stdexcept>
#include <sys/stat.h>
#include <cstdlib>
#include "vector"

namespace FileParser {
    class FileBlockIterator {
    public:
        FileBlockIterator(const char* path, std::size_t blockSize)
            : path_(path), in_(path, std::ios::binary), blockSize_(blockSize), position_(0)
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
        : path_(other.path_), blockSize_(other.blockSize_),
        buffer_(other.buffer_), position_(other.position_)
        {
            in_.open(path_, std::ios::binary);
            if (!in_) {
                throw std::runtime_error("Failed to open file in copy constructor");
            }
            in_.seekg(position_);
        }

        FileBlockIterator& operator=(const FileBlockIterator& other) {
            if(&other == this) {
                return *this;
            }

            if (in_.is_open()) {
                in_.close();
            }

            path_ = other.path_;
            buffer_ = other.buffer_;
            blockSize_ = other.blockSize_;
            position_ = other.position_;
            in_.open(path_, std::ios::binary);
            if (!in_) {
                throw std::runtime_error("Failed to open file in copy assignment");
            }
            in_.seekg(position_);

            return *this;
        }

        FileBlockIterator(FileBlockIterator&& other) noexcept :
        path_(std::move(other.path_)),
        in_(std::move(other.in_)),
        buffer_(std::move(other.buffer_)),
        blockSize_(other.blockSize_),
        position_(other.position_)
        {
            other.blockSize_ = 0;
            other.position_ = 0;
        }

        FileBlockIterator& operator=(FileBlockIterator&& other) noexcept {
            if(this == &other) {
                return *this;
            }

            path_ = std::move(other.path_);
            in_ = std::move(other.in_);
            buffer_ = std::move(other.buffer_);
            blockSize_ = other.blockSize_;
            position_ = other.position_;

            other.blockSize_ = 0;
            other.position_ = 0;

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

        const std::vector<char>& operator*() const { return buffer_; }

        bool operator==(const FileBlockIterator& other) const {
            return position_ == other.position_;
        }

        bool operator!=(const FileBlockIterator& other) const {
            return !(*this == other);
        }



    private:
        void readBlock() {
            if (!in_ || in_.eof()) {
                buffer_.clear();
                return;
            }

            buffer_.assign(blockSize_, '\0');
            in_.read(buffer_.data(), blockSize_);
            buffer_.resize(in_.gcount());
            position_ = in_.tellg();
        }

        std::string path_;
        std::ifstream in_;
        std::vector<char> buffer_;
        std::size_t blockSize_;
        std::streampos position_;
    };


}
