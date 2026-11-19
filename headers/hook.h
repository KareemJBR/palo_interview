#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// hook.dll is injected into the producer (see source/injector.cpp). It has no exported
// API: LoadLibraryW runs DllMain, which installs an Import Address Table hook on
// WriteFile so the ProcessMessage the producer sends over the pipe can be intercepted
// and printed before it is forwarded, unchanged, to the real WriteFile.

using WriteFile_t = BOOL(WINAPI*)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);

// Patches the WriteFile slot in the main module's IAT. Returns false if not found.
bool installHook();
