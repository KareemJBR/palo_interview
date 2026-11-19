#!/usr/bin/env bash
# Builds all three levels. Run from the project root in Git Bash (MinGW g++ on PATH).
set -euo pipefail

FLAGS="-std=c++17 -Wall -Wextra -O2 -Iheaders"

echo "Building producer.exe (generates + sends)..."
g++ $FLAGS source/rules.cpp source/process_message.cpp source/pipe.cpp source/producer.cpp -o producer

echo "Building consumer.exe (receives + prints)..."
g++ $FLAGS source/process_message.cpp source/pipe.cpp source/consumer.cpp -o consumer

echo "Building hook.dll (WriteFile IAT hook)..."
g++ $FLAGS -static -shared source/hook.cpp source/process_message.cpp -o hook.dll

echo "Building injector.exe..."
g++ $FLAGS -static source/injector.cpp -o injector

echo "Building test.exe (unit tests)..."
g++ $FLAGS source/rules.cpp source/process_message.cpp source/pipe.cpp source/test.cpp -o test

echo
echo "Done. Try:"
echo "  ./test        # unit tests"
echo "  ./producer    # Level 1+2: producer -> pipe -> consumer prints"
echo "  ./injector    # Level 3: hooked run, see hook.log"
