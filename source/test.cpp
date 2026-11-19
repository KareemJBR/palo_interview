// Unit tests: rule logic (categories, precedence, boundaries), the ProcessMessage
// wire format, and the pipe layer (round trip, EOF, oversized-header rejection).
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "pipe.h"  // brings <windows.h>; used by the pipe tests
#include "process_message.h"
#include "rules.h"

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool startsWith(const std::string& s, const std::string& prefix) {
    return s.compare(0, prefix.size(), prefix) == 0;
}

std::vector<int> parseNumbers(const std::string& text) {
    std::istringstream in(text);
    std::vector<int> numbers;
    int n;
    while (in >> n) numbers.push_back(n);
    return numbers;
}

const std::string RULE2_TEXT = ": starts with 1 [origin list:";

// The text each number's line should start with, worked out independently of the rules code.
std::string expectedText(int value) {
    if (value % 2 != 0) return ": this is an odd number";
    if (std::to_string(value)[0] == '1') return RULE2_TEXT;
    if (value >= 100) return ": the digit's multiplications are equal to ";
    return ": this number belongs to the others";
}

void checkRule2Line(int value, const std::string& line) {
    const std::string orderedTag = " Ordered list:";
    const auto originPos = line.find(RULE2_TEXT) + RULE2_TEXT.size();
    const auto orderedPos = line.find(orderedTag);
    check(orderedPos != std::string::npos, "rule 2 line has an ordered list");
    if (orderedPos == std::string::npos) return;

    const std::vector<int> origin = parseNumbers(line.substr(originPos, orderedPos - originPos));
    std::string orderedText = line.substr(orderedPos + orderedTag.size());
    if (!orderedText.empty() && orderedText.back() == ']') orderedText.pop_back();
    const std::vector<int> ordered = parseNumbers(orderedText);

    check(static_cast<int>(origin.size()) == value,
          "rule 2 generates " + std::to_string(value) + " numbers");
    check(std::is_sorted(ordered.begin(), ordered.end()), "rule 2 ordered list is sorted");
    check(std::is_permutation(origin.begin(), origin.end(), ordered.begin(), ordered.end()),
          "rule 2 ordered list has the same numbers as the origin list");
}

void testWithXEqualTo5() {
    constexpr int X = 5;
    Rng rng(12345);  // fixed seed so the test is repeatable
    std::ostringstream out;
    runProgram(X, rng, out);

    std::cout << out.str() << '\n';

    std::istringstream lines(out.str());
    std::string header;
    std::getline(lines, header);

    const std::string headerPrefix = "The selected random numbers are:";
    check(startsWith(header, headerPrefix), "header line");
    const std::vector<int> numbers = parseNumbers(header.substr(headerPrefix.size()));
    check(numbers.size() == X, "header lists exactly 5 numbers");

    for (int value : numbers) {
        const std::string v = std::to_string(value);
        check(value >= MIN_VALUE && value <= MAX_VALUE, "number in range: " + v);

        std::string line;
        check(static_cast<bool>(std::getline(lines, line)), "a result line for " + v);
        check(startsWith(line, v + expectedText(value)), "correct rule for " + v + ", got: " + line);

        if (expectedText(value) == RULE2_TEXT) checkRule2Line(value, line);
    }

    std::string extra;
    check(!std::getline(lines, extra), "no extra output lines");
}

void checkRoundTrip(const std::string& payload, const std::string& name) {
    const std::string bytes = ProcessMessage(payload).serialize();
    check(bytes.size() == ProcessMessage::HEADER_SIZE + payload.size(), name + ": serialized size");

    const auto length =
        ProcessMessage::parseHeader(std::string_view(bytes).substr(0, ProcessMessage::HEADER_SIZE));
    check(length && *length == payload.size(), name + ": header holds the payload length");
    check(bytes.substr(ProcessMessage::HEADER_SIZE) == payload, name + ": payload follows the header");
}

void testProcessMessageRoundTrip() {
    checkRoundTrip("", "empty payload");
    checkRoundTrip("311: this is an odd number\n", "one line");
    checkRoundTrip(std::string(5'000'000, 'x'), "large payload");  // header bytes above 0xFF
    checkRoundTrip(std::string("a\0b\0c", 5), "payload with null bytes");  // must be binary-safe
    check(!ProcessMessage::parseHeader("abc"), "header of the wrong size is rejected");
}

// Each number falls into exactly one category; the FIRST matching rule wins. These
// pin the assessment's mapping table and the overlaps where rule order matters.
void testCategories() {
    Rng rng(1);  // only rule 2 consumes it
    // The assessment's own mapping examples: 12->2, 311->1, 222->3, 32->4.
    check(applyRules(311, rng) == "311: this is an odd number", "311 -> rule 1 (odd)");
    check(startsWith(applyRules(222, rng), "222: the digit's multiplications are equal to 8"),
          "222 -> rule 3 (product 8)");
    check(applyRules(32, rng) == "32: this number belongs to the others", "32 -> rule 4");
    check(startsWith(applyRules(12, rng), "12: starts with 1"), "12 -> rule 2");

    // Overlaps: the first matching rule must win.
    check(applyRules(15, rng) == "15: this is an odd number", "odd + starts-1 -> rule 1 beats 2");
    check(applyRules(101, rng) == "101: this is an odd number", "odd + 3-digit + starts-1 -> rule 1");
    check(startsWith(applyRules(150, rng), "150: starts with 1"), "even + 3-digit + starts-1 -> rule 2 beats 3");
    check(startsWith(applyRules(200, rng), "200: the digit"), "even + 3-digit -> rule 3");
}

// Each rule in isolation, including range boundaries and a 3-digit number with a 0 digit.
void testRulesInIsolation() {
    check(rule1(1).has_value() && rule1(999).has_value() && !rule1(2), "rule 1: odd/even + bounds");

    check(rule3(405) && *rule3(405) == "405: the digit's multiplications are equal to 0",
          "rule 3: a 0 digit makes the product 0");
    check(rule3(999) && *rule3(999) == "999: the digit's multiplications are equal to 729",
          "rule 3: max 3-digit");
    check(!rule3(99), "rule 3: rejects a 2-digit number");

    check(rule4(32) == "32: this number belongs to the others", "rule 4: default text");

    Rng rng(7);
    check(!rule2(23, rng), "rule 2: rejects a non-1 leading digit");
    auto one = rule2(1, rng);
    check(one.has_value(), "rule 2: applies to 1");
    if (one) checkRule2Line(1, *one);  // must generate exactly 1 number, sorted, same set
}

// ---- Pipe layer: exercised through a real anonymous pipe, in-process. ----

bool makePipe(HANDLE& read, HANDLE& write) {
    SECURITY_ATTRIBUTES attrs{sizeof(SECURITY_ATTRIBUTES), nullptr, FALSE};
    return CreatePipe(&read, &write, &attrs, 0);
}

void testPipeRoundTrip() {
    HANDLE read = nullptr, write = nullptr;
    check(makePipe(read, write), "CreatePipe");
    if (!read) return;

    // Small payload: it fits the pipe buffer, so the single-threaded write never blocks.
    const ProcessMessage sent("line1\nline2\n");
    check(sendMessage(write, sent), "sendMessage over a pipe");
    const auto got = receiveMessage(read);
    check(got && got->payload() == sent.payload(), "pipe round trip: payload matches");

    CloseHandle(read);
    CloseHandle(write);
}

void testPipeEof() {
    HANDLE read = nullptr, write = nullptr;
    check(makePipe(read, write), "CreatePipe (EOF)");
    if (!read) return;

    CloseHandle(write);  // writer gone before anything is sent
    check(!receiveMessage(read), "EOF -> nullopt (no hang)");
    CloseHandle(read);
}

void testPipeOversizedHeader() {
    HANDLE read = nullptr, write = nullptr;
    check(makePipe(read, write), "CreatePipe (oversized)");
    if (!read) return;

    const std::uint32_t tooBig = ProcessMessage::MAX_PAYLOAD + 1;
    char header[ProcessMessage::HEADER_SIZE];
    for (std::size_t i = 0; i < ProcessMessage::HEADER_SIZE; ++i) {
        header[i] = static_cast<char>((tooBig >> (8 * i)) & 0xFF);
    }
    DWORD wrote = 0;
    WriteFile(write, header, sizeof(header), &wrote, nullptr);
    CloseHandle(write);
    // Must reject on the length check, without allocating the payload or blocking.
    check(!receiveMessage(read), "oversized header -> nullopt");
    CloseHandle(read);
}

}  // namespace

int main() {
    testWithXEqualTo5();
    testCategories();
    testRulesInIsolation();
    testProcessMessageRoundTrip();
    testPipeRoundTrip();
    testPipeEof();
    testPipeOversizedHeader();

    if (failures == 0) {
        std::cout << "PASS: rules (X=5, categories, precedence, boundaries)\n";
        std::cout << "PASS: ProcessMessage round trip\n";
        std::cout << "PASS: pipe (round trip, EOF, oversized header)\n";
        return 0;
    }
    std::cout << failures << " check(s) failed\n";
    return 1;
}
