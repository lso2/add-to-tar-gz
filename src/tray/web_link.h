// web_link.h: opens a web address in the default browser.
// Globals: OpenWebPage.
// Doc: docs/notes/src/tray/web_link.md
#pragma once
#include <windows.h>

// Opens `url` with ShellExecuteExW. A failure is shown in a message box owned by `owner`, with Windows' text.
void OpenWebPage(HWND owner, const wchar_t* url);
