// injector.exe: starts Process 1 suspended, injects hook.dll so the WriteFile hook
// is in place before Process 1 runs, then resumes it. process1.exe and hook.dll are
// expected next to injector.exe.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <iostream>
#include <string>

namespace {

std::wstring dirOfThisExe() {
    wchar_t buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    std::wstring path(buffer, length);
    path.erase(path.find_last_of(L"\\/") + 1);
    return path;
}

// Loads hook.dll inside the target by running LoadLibraryW there via a remote thread.
bool injectDll(HANDLE process, const std::wstring& dllPath) {
    const SIZE_T bytes = (dllPath.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) return false;

    bool ok = WriteProcessMemory(process, remote, dllPath.c_str(), bytes, nullptr);
    if (ok) {
        auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(
            GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
        HANDLE thread = CreateRemoteThread(process, nullptr, 0, loadLibrary, remote, 0, nullptr);
        if (thread) {
            WaitForSingleObject(thread, INFINITE);
            DWORD loaded = 0;
            GetExitCodeThread(thread, &loaded);  // 0 on x64 => LoadLibraryW failed
            ok = loaded != 0;
            CloseHandle(thread);
        } else {
            ok = false;
        }
    }
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    return ok;
}

}  // namespace

int main() {
    const std::wstring dir = dirOfThisExe();
    const std::wstring process1 = dir + L"process1.exe";
    const std::wstring dll = dir + L"hook.dll";

    std::wstring cmdline = L"\"" + process1 + L"\"";  // CreateProcessW may modify this buffer
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION info{};
    if (!CreateProcessW(process1.c_str(), cmdline.data(), nullptr, nullptr, TRUE, CREATE_SUSPENDED,
                        nullptr, nullptr, &startup, &info)) {
        std::cerr << "injector: failed to start process1 (error " << GetLastError() << ")\n";
        return 1;
    }

    if (!injectDll(info.hProcess, dll)) {
        std::cerr << "injector: failed to inject hook.dll (error " << GetLastError() << ")\n";
        TerminateProcess(info.hProcess, 1);
        CloseHandle(info.hThread);
        CloseHandle(info.hProcess);
        return 1;
    }

    ResumeThread(info.hThread);
    WaitForSingleObject(info.hProcess, INFINITE);

    DWORD exitCode = 1;
    GetExitCodeProcess(info.hProcess, &exitCode);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);
    return static_cast<int>(exitCode);
}
