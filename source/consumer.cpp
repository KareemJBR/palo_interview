// Consumer: receives a ProcessMessage from the producer through its stdin pipe and prints it.
#include <iostream>

#include "pipe.h"

int main() {
    const auto message = receiveMessage(GetStdHandle(STD_INPUT_HANDLE));
    if (!message) {
        std::cerr << "consumer: failed to receive a message from the producer\n";
        return 1;
    }

    std::cout << message->payload() << std::flush;
    return 0;
}
