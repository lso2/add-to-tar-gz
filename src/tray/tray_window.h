// tray_window.h: the hidden top-level window of AddToTarGzTray.exe. It owns the tray icon and the settings
// window, and Installer.iss and Restart Manager close the running app through it.
// Globals: kTrayWindowClass, WM_ATG_* messages, CreateTrayWindow, TrayDialog.
// Doc: docs/notes/src/tray/tray_window.md
#pragma once
#include <windows.h>

// Installer.iss finds the running app by this class name and closes it with WM_CLOSE.
inline constexpr wchar_t kTrayWindowClass[] = L"AddToTarGzTrayWindow";

inline constexpr UINT WM_ATG_TRAY = WM_APP + 1;              // notification area events of the icon
inline constexpr UINT WM_ATG_SHOW_SETTINGS = WM_APP + 2;     // a second start asks for the settings window
inline constexpr UINT WM_ATG_RELOAD = WM_APP + 3;            // to the settings window: the tray menu changed a value
inline constexpr UINT WM_ATG_SETTINGS_APPLIED = WM_APP + 4;  // the settings window saved its values
inline constexpr UINT WM_ATG_SETTINGS_CLOSED = WM_APP + 5;   // the settings window was destroyed
// lParam is an UpdateResult*. The update check posts it to the tray window. The tray window sends it on to an
// open settings window, then deletes it.
inline constexpr UINT WM_ATG_UPDATE_RESULT = WM_APP + 6;

// Creates the hidden window and shows the tray icon when the TrayIcon setting is on. With `openSettings` set, the
// settings window is opened as well. Returns nullptr when the window class or the window cannot be created.
HWND CreateTrayWindow(HINSTANCE instance, bool openSettings);

// The open settings window or nullptr, for IsDialogMessageW in the message loop.
HWND TrayDialog();
