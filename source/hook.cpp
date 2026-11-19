// hook.dll: an Import Address Table hook on WriteFile, installed in Process 1 by
// the injector. It reconstructs each ProcessMessage from the bytes written to the
// pipe, prints it, then calls the real WriteFile so Process 1 -> Process 2 is
// unaffected. This is the API-interception technique endpoint-security agents use.
#include "hook.h"

#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

#include "process_message.h"

namespace {

WriteFile_t g_realWriteFile = nullptr;
std::mutex g_mutex;
std::string g_accum;   // bytes seen on the pipe, awaiting a complete message
std::ofstream g_log;

void logMessage(const ProcessMessage& message) {
    const std::string banner = "[hook] intercepted ProcessMessage (" +
                               std::to_string(ProcessMessage::HEADER_SIZE + message.payload().size()) +
                               " bytes):\n" + message.payload();
    if (g_log) g_log << banner << "\n--- end of message ---\n" << std::flush;
    // Also to stderr so it is visible on the console during a run.
    fprintf(stderr, "%s\n", banner.c_str());
    fflush(stderr);
}

// Pulls every complete message out of the accumulator.
void drainMessages() {
    for (;;) {
        if (g_accum.size() < ProcessMessage::HEADER_SIZE) return;
        const auto length = ProcessMessage::parseHeader(
            std::string_view(g_accum).substr(0, ProcessMessage::HEADER_SIZE));
        if (!length) return;

        const std::size_t total = ProcessMessage::HEADER_SIZE + *length;
        if (g_accum.size() < total) return;  // wait for the rest of the payload

        logMessage(ProcessMessage(g_accum.substr(ProcessMessage::HEADER_SIZE, *length)));
        g_accum.erase(0, total);
    }
}

// The replacement for WriteFile. Forwards every call unchanged; for pipe writes it
// also accumulates the bytes and prints any complete ProcessMessage.
BOOL WINAPI hookedWriteFile(HANDLE handle, LPCVOID buffer, DWORD toWrite, LPDWORD written,
                            LPOVERLAPPED overlapped) {
    const WriteFile_t real = g_realWriteFile ? g_realWriteFile : &WriteFile;
    const BOOL ok = real(handle, buffer, toWrite, written, overlapped);

    if (ok && buffer != nullptr && GetFileType(handle) == FILE_TYPE_PIPE) {
        const DWORD count = written ? *written : toWrite;
        std::lock_guard<std::mutex> lock(g_mutex);
        g_accum.append(static_cast<const char*>(buffer), count);
        drainMessages();
    }
    return ok;
}

}  // namespace

bool installHook() {
    auto* base = reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);

    const auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (dir.VirtualAddress == 0) return false;
    auto* desc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + dir.VirtualAddress);

    // Scan every imported DLL for a function named "WriteFile" and patch its IAT slot.
    for (; desc->Name != 0; ++desc) {
        if (desc->OriginalFirstThunk == 0) continue;
        auto* names = reinterpret_cast<IMAGE_THUNK_DATA*>(base + desc->OriginalFirstThunk);
        auto* iat = reinterpret_cast<IMAGE_THUNK_DATA*>(base + desc->FirstThunk);

        for (; names->u1.AddressOfData != 0; ++names, ++iat) {
            if (names->u1.Ordinal & IMAGE_ORDINAL_FLAG) continue;  // imported by ordinal
            auto* byName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + names->u1.AddressOfData);
            if (std::string_view(byName->Name) != "WriteFile") continue;

            void** slot = reinterpret_cast<void**>(&iat->u1.Function);
            DWORD oldProtect = 0;
            if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &oldProtect)) return false;
            g_realWriteFile = reinterpret_cast<WriteFile_t>(*slot);
            *slot = reinterpret_cast<void*>(&hookedWriteFile);
            VirtualProtect(slot, sizeof(void*), oldProtect, &oldProtect);
            return true;
        }
    }
    return false;
}

extern "C" BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_log.open("hook.log", std::ios::out | std::ios::trunc);
        const bool ok = installHook();
        if (g_log) g_log << (ok ? "[hook] installed; WriteFile hooked\n"
                                : "[hook] FAILED to find WriteFile in the import table\n")
                         << std::flush;
        fprintf(stderr, ok ? "[hook] installed; WriteFile hooked\n"
                           : "[hook] FAILED to find WriteFile in the import table\n");
        fflush(stderr);
    }
    return TRUE;
}
