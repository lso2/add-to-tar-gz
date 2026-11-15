// settings_dialog.h: the modeless AddToTarGz Settings window.
// Globals: OpenSettingsDialog.
// Doc: docs/notes/src/tray/settings_dialog.md
#pragma once
#include <windows.h>

// Creates the window from IDD_SETTINGS with the current settings. `trayWindow` receives WM_ATG_SETTINGS_APPLIED
// after each save and WM_ATG_SETTINGS_CLOSED when the window is destroyed. Returns nullptr when creation fails.
HWND OpenSettingsDialog(HINSTANCE instance, HWND trayWindow);
