#include "pipe.h"

#include <algorithm>
#include <limits>

bool writeAll(HANDLE handle, std::string_view data) {
    constexpr std::size_t MAX_CHUNK = std::numeric_limits<DWORD>::max();

    while (!data.empty()) {
        const auto chunk = static_cast<DWORD>(std::min(data.size(), MAX_CHUNK));
        DWORD written = 0;
        if (!WriteFile(handle, data.data(), chunk, &written, nullptr)) return false;
        if (written == 0) return false;  // no progress -> avoid spinning forever
        data.remove_prefix(written);
    }
    return true;
}

std::optional<std::string> readExact(HANDLE handle, std::size_t count) {
    constexpr std::size_t MAX_CHUNK = std::numeric_limits<DWORD>::max();

    std::string buffer(count, '\0');
    std::size_t received = 0;
    while (received < count) {
        const auto chunk = static_cast<DWORD>(std::min(count - received, MAX_CHUNK));
        DWORD read = 0;
        if (!ReadFile(handle, buffer.data() + received, chunk, &read, nullptr) || read == 0) {
            return std::nullopt;  // error, or the writer closed the pipe early
        }
        received += read;
    }
    return buffer;
}

bool sendMessage(HANDLE handle, const ProcessMessage& message) {
    return writeAll(handle, message.serialize());
}

std::optional<ProcessMessage> receiveMessage(HANDLE handle) {
    const auto header = readExact(handle, ProcessMessage::HEADER_SIZE);
    if (!header) return std::nullopt;

    const auto length = ProcessMessage::parseHeader(*header);
    if (!length) return std::nullopt;
    if (*length > ProcessMessage::MAX_PAYLOAD) return std::nullopt;  // reject an absurd length

    auto payload = readExact(handle, *length);
    if (!payload) return std::nullopt;

    return ProcessMessage(std::move(*payload));
}
