// autostart.h: the HKCU Run value that starts the tray app at sign-in.
// Globals: SetStartAtSignIn.
// Doc: docs/notes/src/tray/autostart.md
#pragma once
#include <string>

// Writes "<this exe>" --tray as the value AddToTarGz under HKCU\Software\Microsoft\Windows\CurrentVersion\Run,
// or deletes that value. Returns false with the Windows error text when the registry call fails.
bool SetStartAtSignIn(bool enabled, std::wstring& error);
