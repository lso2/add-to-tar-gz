// update_check.h: asks GitHub for the latest AddToTarGz release on a background thread.
// Globals: StartUpdateCheck.
// Doc: docs/notes/src/tray/update_check.md
#pragma once
#include <windows.h>
#include <string>

// Starts the check on a new thread. At its end `notify` is posted `message` with a new UpdateResult* in lParam,
// and the window handling the message deletes it. Returns false with `error` set when the thread cannot start.
bool StartUpdateCheck(HWND notify, UINT message, std::wstring& error);
