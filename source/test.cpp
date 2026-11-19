// Test: run the program with X forced to 5 and check every output line.
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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
    check(!ProcessMessage::parseHeader("abc"), "header of the wrong size is rejected");
}

}  // namespace

int main() {
    testWithXEqualTo5();
    testProcessMessageRoundTrip();
    if (failures == 0) {
        std::cout << "PASS: test with X = 5\n";
        std::cout << "PASS: ProcessMessage round trip\n";
        return 0;
    }
    std::cout << failures << " check(s) failed\n";
    return 1;
}
