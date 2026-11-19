#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "process_message.h"

// Owns a Win32 HANDLE and closes it on destruction. Movable, not copyable.
class UniqueHandle {
public:
    UniqueHandle() = default;
    explicit UniqueHandle(HANDLE handle) : handle_(handle) {}
    ~UniqueHandle() { reset(); }

    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&& other) noexcept : handle_(other.release()) {}
    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        if (this != &other) {
            reset();
            handle_ = other.release();
        }
        return *this;
    }

    HANDLE get() const { return handle_; }
    bool valid() const { return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE; }

    HANDLE release() {
        HANDLE h = handle_;
        handle_ = nullptr;
        return h;
    }

    void reset() {
        if (valid()) CloseHandle(handle_);
        handle_ = nullptr;
    }

private:
    HANDLE handle_ = nullptr;
};

// Writes every byte of `data`; false on error.
bool writeAll(HANDLE handle, std::string_view data);

// Reads exactly `count` bytes; nullopt on EOF or error.
std::optional<std::string> readExact(HANDLE handle, std::size_t count);

// All messages go through these two functions.
bool sendMessage(HANDLE handle, const ProcessMessage& message);
std::optional<ProcessMessage> receiveMessage(HANDLE handle);
