// tray_menu.h: the tray icon's right-click menu and the setting each choice writes.
// Globals: TrayMenuResult, ShowTrayMenu.
// Doc: docs/notes/src/tray/tray_menu.md
#pragma once
#include <windows.h>

enum class TrayMenuResult { None, OpenSettings, SettingsChanged, BuyCoffee, Exit };

// Shows the menu at `point` with the current settings checked. A settings choice is written before this returns.
TrayMenuResult ShowTrayMenu(HWND window, POINT point);
