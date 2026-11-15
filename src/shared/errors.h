// errors.h: turns a Windows error code into the system's own message text.
// Globals: WindowsErrorText.
// Doc: docs/notes/src/shared/errors.md
#pragma once
#include <windows.h>
#include <string>

// System message for the code, followed by "(error N)". The text is only what Windows reports.
std::wstring WindowsErrorText(DWORD code);
