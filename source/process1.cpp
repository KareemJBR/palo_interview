// Process 1: runs the program, then sends its output to Process 2 through an anonymous pipe.
// Process 1 never prints the output itself; only errors go to stderr.
#include <iostream>
#include <sstream>
#include <string>

#include "pipe.h"
#include "process_message.h"
#include "rules.h"

namespace {

// process2.exe is expected next to process1.exe, so it is found from any working directory.
std::wstring receiverPath() {
    wchar_t buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    std::wstring path(buffer, length);
    path.erase(path.find_last_of(L"\\/") + 1);
    return path + L"process2.exe";
}

struct Receiver {
    UniqueHandle process;
    UniqueHandle pipeWriteEnd;
};

// Creates the pipe and starts Process 2 with the pipe's read end as its stdin.
std::optional<Receiver> startReceiver() {
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

    std::wstring path = receiverPath();
    PROCESS_INFORMATION info{};
    if (!CreateProcessW(path.c_str(), nullptr, nullptr, nullptr, TRUE, 0, nullptr, nullptr,
                        &startup, &info)) {
        return std::nullopt;
    }
    CloseHandle(info.hThread);

    // `read` is closed when it goes out of scope; the child keeps its own copy.
    return Receiver{UniqueHandle(info.hProcess), std::move(write)};
}

}  // namespace

int main() {
    Rng rng(std::random_device{}());
    const int count = std::uniform_int_distribution<int>(MIN_VALUE, MAX_VALUE)(rng);

    std::ostringstream output;
    runProgram(count, rng, output);

    auto receiver = startReceiver();
    if (!receiver) {
        std::cerr << "process1: failed to start process2 (error " << GetLastError() << ")\n";
        return 1;
    }

    const bool sent = sendMessage(receiver->pipeWriteEnd.get(), ProcessMessage(output.str()));
    receiver->pipeWriteEnd.reset();  // signals EOF to Process 2
    if (!sent) {
        std::cerr << "process1: failed to send the message to process2\n";
    }

    WaitForSingleObject(receiver->process.get(), INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(receiver->process.get(), &exitCode);
    return sent ? static_cast<int>(exitCode) : 1;
}
