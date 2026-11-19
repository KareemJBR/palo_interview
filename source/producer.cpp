// Producer: runs the program, then sends its output to the consumer through an
// anonymous pipe. The producer never prints the output itself; only errors go to stderr.
#include <iostream>
#include <sstream>
#include <string>

#include "pipe.h"
#include "process_message.h"
#include "rules.h"

namespace {

// consumer.exe is expected next to producer.exe, so it is found from any working directory.
std::wstring consumerPath() {
    wchar_t buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    std::wstring path(buffer, length);
    path.erase(path.find_last_of(L"\\/") + 1);
    return path + L"consumer.exe";
}

struct Consumer {
    UniqueHandle process;
    UniqueHandle pipeWriteEnd;
};

// Creates the pipe and starts the consumer with the pipe's read end as its stdin.
std::optional<Consumer> startConsumer() {
    SECURITY_ATTRIBUTES inheritable{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};

    HANDLE readEnd = nullptr;
    HANDLE writeEnd = nullptr;
    if (!CreatePipe(&readEnd, &writeEnd, &inheritable, 0)) return std::nullopt;
    UniqueHandle read(readEnd);
    UniqueHandle write(writeEnd);

    // Only the read end goes to the child. If the child inherited the write end too,
    // it would never see EOF on its stdin.
    if (!SetHandleInformation(write.get(), HANDLE_FLAG_INHERIT, 0)) return std::nullopt;

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = read.get();
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    std::wstring path = consumerPath();
    PROCESS_INFORMATION info{};
    if (!CreateProcessW(path.c_str(), nullptr, nullptr, nullptr, TRUE, 0, nullptr, nullptr,
                        &startup, &info)) {
        return std::nullopt;
    }
    CloseHandle(info.hThread);

    // `read` is closed when it goes out of scope; the child keeps its own copy.
    return Consumer{UniqueHandle(info.hProcess), std::move(write)};
}

}  // namespace

int main() {
    Rng rng(std::random_device{}());
    const int count = std::uniform_int_distribution<int>(MIN_VALUE, MAX_VALUE)(rng);

    std::ostringstream output;
    runProgram(count, rng, output);

    auto consumer = startConsumer();
    if (!consumer) {
        std::cerr << "producer: failed to start consumer (error " << GetLastError() << ")\n";
        return 1;
    }

    const bool sent = sendMessage(consumer->pipeWriteEnd.get(), ProcessMessage(output.str()));
    consumer->pipeWriteEnd.reset();  // signals EOF to the consumer
    if (!sent) {
        std::cerr << "producer: failed to send the message to the consumer\n";
    }

    WaitForSingleObject(consumer->process.get(), INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(consumer->process.get(), &exitCode);
    return sent ? static_cast<int>(exitCode) : 1;
}
