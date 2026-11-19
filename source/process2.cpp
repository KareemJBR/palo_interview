// Process 2: receives a ProcessMessage from Process 1 through its stdin pipe and prints it.
#include <iostream>

#include "pipe.h"

int main() {
    const auto message = receiveMessage(GetStdHandle(STD_INPUT_HANDLE));
    if (!message) {
        std::cerr << "process2: failed to receive a message from process1\n";
        return 1;
    }

    std::cout << message->payload() << std::flush;
    return 0;
}
