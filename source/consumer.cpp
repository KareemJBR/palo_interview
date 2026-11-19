// Consumer: receives a ProcessMessage from the producer through its stdin pipe and prints it.
#include <iostream>

#include "pipe.h"

int main() {
    const HANDLE stdin_ = GetStdHandle(STD_INPUT_HANDLE);

    // The consumer is meant to be launched by the producer with a pipe on its stdin.
    // If stdin is a console, there is no producer, so reading would just block on the
    // keyboard with no prompt. Fail clearly instead.
    if (GetFileType(stdin_) == FILE_TYPE_CHAR) {
        std::cerr << "consumer: no producer -- run ./producer (or ./injector); "
                     "this program expects a message on a pipe, not the keyboard\n";
        return 2;
    }

    const auto message = receiveMessage(stdin_);
    if (!message) {
        std::cerr << "consumer: failed to receive a message from the producer\n";
        return 1;
    }

    std::cout << message->payload() << std::flush;
    return 0;
}
