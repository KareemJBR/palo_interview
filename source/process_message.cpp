#include "process_message.h"

std::string ProcessMessage::serialize() const {
    const auto length = static_cast<std::uint32_t>(payload_.size());

    std::string bytes;
    bytes.reserve(HEADER_SIZE + payload_.size());
    for (std::size_t i = 0; i < HEADER_SIZE; ++i) {
        bytes += static_cast<char>((length >> (8 * i)) & 0xFF);
    }
    bytes += payload_;
    return bytes;
}

std::optional<std::uint32_t> ProcessMessage::parseHeader(std::string_view header) {
    if (header.size() != HEADER_SIZE) return std::nullopt;

    std::uint32_t length = 0;
    for (std::size_t i = 0; i < HEADER_SIZE; ++i) {
        length |= static_cast<std::uint32_t>(static_cast<unsigned char>(header[i])) << (8 * i);
    }
    return length;
}
