// web_link.cpp: ShellExecuteExW for the links of the settings window and the tray menu.
// Globals: none beyond web_link.h.
// Doc: docs/notes/src/tray/web_link.md
#include "web_link.h"
#include "errors.h"
#include <shellapi.h>
#include <string>

void OpenWebPage(HWND owner, const wchar_t* url)
{
    SHELLEXECUTEINFOW info = {};
    info.cbSize = sizeof(info);
    // Windows' own error dialog is suppressed. The message box below shows the same error text.
    info.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
    info.hwnd = owner;
    info.lpVerb = L"open";
    info.lpFile = url;
    info.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&info)) {
        const std::wstring text = L"Opening " + std::wstring(url) + L" failed: " + WindowsErrorText(GetLastError());
        MessageBoxW(owner, text.c_str(), L"AddToTarGz", MB_OK | MB_ICONERROR);
    }
}
