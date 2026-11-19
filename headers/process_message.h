#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// A message sent from Process 1 to Process 2.
// Wire format: 4-byte little-endian payload length, followed by the payload bytes.
class ProcessMessage {
public:
    static constexpr std::size_t HEADER_SIZE = sizeof(std::uint32_t);

    explicit ProcessMessage(std::string payload) : payload_(std::move(payload)) {}

    const std::string& payload() const { return payload_; }

    // Header + payload, ready to write to a pipe.
    std::string serialize() const;

    // Reads the payload length from a header; nullopt if the header is the wrong size.
    static std::optional<std::uint32_t> parseHeader(std::string_view header);

private:
    std::string payload_;
};
