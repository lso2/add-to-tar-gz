// pipes.cpp: pipes, NUL handles and the byte relay between 7-Zip stages.
// Globals: none beyond pipes.h.
// Doc: docs/notes/src/worker/pipes.md
#include "pipes.h"
#include "errors.h"
#include <vector>

bool MakePipe(Pipe& pipe, bool childReads, bool childWrites, std::wstring& error)
{
    SECURITY_ATTRIBUTES attributes = {sizeof(attributes), nullptr, TRUE};
    if (!CreatePipe(&pipe.read, &pipe.write, &attributes, 1 << 20)) {
        error = L"CreatePipe failed: " + WindowsErrorText(GetLastError());
        return false;
    }
    SetHandleInformation(pipe.read, HANDLE_FLAG_INHERIT, childReads ? HANDLE_FLAG_INHERIT : 0);
    SetHandleInformation(pipe.write, HANDLE_FLAG_INHERIT, childWrites ? HANDLE_FLAG_INHERIT : 0);
    return true;
}

HANDLE OpenNul(std::wstring& error)
{
    SECURITY_ATTRIBUTES attributes = {sizeof(attributes), nullptr, TRUE};
    HANDLE nul = CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &attributes,
                             OPEN_EXISTING, 0, nullptr);
    if (nul == INVALID_HANDLE_VALUE) {
        error = L"Opening NUL failed: " + WindowsErrorText(GetLastError());
        return nullptr;
    }
    return nul;
}

DWORD RelayBytes(HANDLE from, HANDLE to, std::atomic<uint64_t>& counter, const std::atomic<bool>& cancel)
{
    const bool fromPipe = GetFileType(from) == FILE_TYPE_PIPE;
    std::vector<char> buffer(256 * 1024);
    for (;;) {
        if (cancel.load()) {
            return ERROR_CANCELLED;
        }
        DWORD got = 0;
        if (!ReadFile(from, buffer.data(), static_cast<DWORD>(buffer.size()), &got, nullptr)) {
            const DWORD code = GetLastError();
            // ERROR_BROKEN_PIPE is the end-of-input signal of a pipe whose writing process closed it.
            return code == ERROR_BROKEN_PIPE ? 0 : code;
        }
        if (got == 0) {
            if (fromPipe) {
                continue;
            }
            return 0;
        }
        DWORD offset = 0;
        while (offset < got) {
            DWORD put = 0;
            if (!WriteFile(to, buffer.data() + offset, got - offset, &put, nullptr)) {
                return GetLastError();
            }
            offset += put;
        }
        counter += got;
    }
}

void CloseIfOpen(HANDLE& handle)
{
    if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
    }
    handle = nullptr;
}
