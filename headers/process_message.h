#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// A message sent from the producer to the consumer.
// Wire format: 4-byte little-endian payload length, followed by the payload bytes.
class ProcessMessage {
public:
    static constexpr std::size_t HEADER_SIZE = sizeof(std::uint32_t);

    // A receiver rejects any header claiming a payload larger than this. The real
    // output is a few MB at most; this only guards against a garbage/hostile header
    // (e.g. run by hand with junk on stdin) triggering a huge allocation.
    static constexpr std::uint32_t MAX_PAYLOAD = 64u * 1024u * 1024u;  // 64 MiB

    explicit ProcessMessage(std::string payload) : payload_(std::move(payload)) {}

    const std::string& payload() const { return payload_; }

    // Header + payload, ready to write to a pipe.
    std::string serialize() const;

    // Reads the payload length from a header; nullopt if the header is the wrong size.
    static std::optional<std::uint32_t> parseHeader(std::string_view header);

private:
    std::string payload_;
};
