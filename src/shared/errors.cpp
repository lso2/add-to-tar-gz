// errors.cpp: system message text for Windows error codes.
// Globals: none beyond errors.h.
// Doc: docs/notes/src/shared/errors.md
#include "errors.h"

std::wstring WindowsErrorText(DWORD code)
{
    wchar_t* buffer = nullptr;
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, 0, reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    std::wstring text;
    if (length != 0 && buffer != nullptr) {
        text.assign(buffer, length);
        LocalFree(buffer);
        while (!text.empty() && (text.back() == L'\r' || text.back() == L'\n' || text.back() == L' ')) {
            text.pop_back();
        }
    }
    return text + L" (error " + std::to_wstring(code) + L")";
}
