#!/usr/bin/env bash
# Builds all three levels. Run from the project root in Git Bash (MinGW g++ on PATH).
set -euo pipefail

FLAGS="-std=c++17 -Wall -Wextra -O2 -Iheaders"

echo "Building process1.exe (sender)..."
g++ $FLAGS source/rules.cpp source/process_message.cpp source/pipe.cpp source/process1.cpp -o process1

echo "Building process2.exe (receiver)..."
g++ $FLAGS source/process_message.cpp source/pipe.cpp source/process2.cpp -o process2

echo "Building hook.dll (WriteFile IAT hook)..."
g++ $FLAGS -static -shared source/hook.cpp source/process_message.cpp -o hook.dll

echo "Building injector.exe..."
g++ $FLAGS -static source/injector.cpp -o injector

echo "Building test.exe (unit tests)..."
g++ $FLAGS source/rules.cpp source/process_message.cpp source/test.cpp -o test

echo
echo "Done. Try:"
echo "  ./test        # unit tests"
echo "  ./process1    # Level 1+2: Process 1 -> pipe -> Process 2 prints"
echo "  ./injector    # Level 3: hooked run, see hook.log"
