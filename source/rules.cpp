#include "rules.h"

#include <algorithm>
#include <ostream>

namespace {

void appendNumbers(std::string& out, const std::vector<int>& numbers) {
    for (int n : numbers) {
        out += ' ';
        out += std::to_string(n);
    }
}

int leadingDigit(int value) {
    while (value >= 10) value /= 10;
    return value;
}

}  // namespace

std::vector<int> generateNumbers(int count, Rng& rng) {
    std::uniform_int_distribution<int> dist(MIN_VALUE, MAX_VALUE);
    std::vector<int> numbers;
    numbers.reserve(count);
    for (int i = 0; i < count; ++i) {
        numbers.push_back(dist(rng));
    }
    return numbers;
}

// Rule 1: odd numbers.
std::optional<std::string> rule1(int value) {
    if (value % 2 == 0) return std::nullopt;
    return std::to_string(value) + ": this is an odd number";
}

// Rule 2: numbers whose first digit is 1.
// Generates `value` random numbers, prints them as generated and in ascending order.
std::optional<std::string> rule2(int value, Rng& rng) {
    if (leadingDigit(value) != 1) return std::nullopt;

    std::vector<int> origin = generateNumbers(value, rng);
    std::vector<int> ordered = origin;
    std::sort(ordered.begin(), ordered.end());

    std::string out = std::to_string(value) + ": starts with 1 [origin list:";
    appendNumbers(out, origin);
    out += " Ordered list:";
    appendNumbers(out, ordered);
    out += ']';
    return out;
}

// Rule 3: numbers with exactly 3 digits. Prints the product of the digits.
std::optional<std::string> rule3(int value) {
    if (value < 100 || value > 999) return std::nullopt;

    int product = 1;
    for (int n = value; n > 0; n /= 10) {
        product *= n % 10;
    }
    return std::to_string(value) + ": the digit's multiplications are equal to " +
           std::to_string(product);
}

// Rule 4 (default): everything else.
std::string rule4(int value) {
    return std::to_string(value) + ": this number belongs to the others";
}

std::string applyRules(int value, Rng& rng) {
    if (auto out = rule1(value)) return *out;
    if (auto out = rule2(value, rng)) return *out;
    if (auto out = rule3(value)) return *out;
    return rule4(value);
}

void runProgram(int count, Rng& rng, std::ostream& out) {
    const std::vector<int> numbers = generateNumbers(count, rng);

    std::string header = "The selected random numbers are:";
    appendNumbers(header, numbers);
    out << header << '\n';

    for (int value : numbers) {
        out << applyRules(value, rng) << '\n';
    }
}
