#pragma once

#include <iosfwd>
#include <optional>
#include <random>
#include <string>
#include <vector>

constexpr int MIN_VALUE = 1;
constexpr int MAX_VALUE = 999;

using Rng = std::mt19937;

std::vector<int> generateNumbers(int count, Rng& rng);

// Rules 1-3 return the output line if the rule applies, std::nullopt otherwise.
// Rule 4 is the default and always applies.
std::optional<std::string> rule1(int value);
std::optional<std::string> rule2(int value, Rng& rng);
std::optional<std::string> rule3(int value);
std::string rule4(int value);

// Rules are checked in order. The first one that applies wins; otherwise rule 4 is used.
std::string applyRules(int value, Rng& rng);

// Generates `count` numbers, prints the header line and one result line per number.
void runProgram(int count, Rng& rng, std::ostream& out);
