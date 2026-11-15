// tray_icon.h: the AddToTarGz icon in the notification area.
// Globals: AddTrayIcon, RemoveTrayIcon.
// Doc: docs/notes/src/tray/tray_icon.md
#pragma once
#include <windows.h>

// Adds the icon with NOTIFYICON_VERSION_4. Its events are sent to `window` as WM_ATG_TRAY. Returns false while the
// taskbar does not exist yet, as at sign-in before Explorer starts.
bool AddTrayIcon(HWND window, HINSTANCE instance);

void RemoveTrayIcon(HWND window);
