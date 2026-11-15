// pipes.h: anonymous pipes that chain the 7-Zip stages, the NUL device, and a counting byte relay.
// Globals: Pipe, MakePipe, OpenNul, RelayBytes, CloseIfOpen.
// Doc: docs/notes/src/worker/pipes.md
#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <string>

struct Pipe {
    HANDLE read = nullptr;
    HANDLE write = nullptr;
};

// Creates a pipe with a 1 MB buffer. An end is inheritable only when a child receives it.
bool MakePipe(Pipe& pipe, bool childReads, bool childWrites, std::wstring& error);

// Inheritable handle to NUL, for a child stream that carries nothing.
HANDLE OpenNul(std::wstring& error);

// Copies `from` into `to` until the end of input or a cancel, adding each chunk to `counter`.
// Returns 0 at the end of input, ERROR_CANCELLED on cancel, or the Windows error of a failed read or write.
DWORD RelayBytes(HANDLE from, HANDLE to, std::atomic<uint64_t>& counter, const std::atomic<bool>& cancel);

// Closes the handle when it is open and clears it.
void CloseIfOpen(HANDLE& handle);
