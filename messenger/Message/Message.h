#pragma once
#include "iostream"
#include "string"

namespace CPCDMessenger {
    class Message {
    public:

        explicit Message(const std::string& text) : text_(text) {};

    private:

        std::string text_;

    };
} // CPCDMessenger

