// tray_menu.cpp: builds the tray menu from the settings and writes the setting a choice changes.
// Globals: none beyond tray_menu.h.
// Doc: docs/notes/src/tray/tray_menu.md
#include "tray_menu.h"
#include "coffee_icon.h"
#include "settings.h"
#include "tray_ids.h"
#include <shellscalingapi.h>
#include <string>

namespace {

UINT LevelCommand(int level)
{
    return level == 1 ? IDM_LEVEL_1 : (level == 9 ? IDM_LEVEL_9 : IDM_LEVEL_5);
}

// The small icon size at the DPI of the monitor the menu opens on. Menu item bitmaps are not scaled by Windows.
int MenuIconSize(POINT point)
{
    UINT dpiX = 0;
    UINT dpiY = 0;
    HMONITOR monitor = MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST);
    if (FAILED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
        dpiX = GetDpiForSystem();
    }
    return GetSystemMetricsForDpi(SM_CXSMICON, dpiX);
}

void AddCoffeeItem(HMENU menu, HBITMAP coffee)
{
    AppendMenuW(menu, MF_STRING, IDM_COFFEE, L"Buy me a coffee");
    if (coffee == nullptr) {
        return;
    }
    MENUITEMINFOW item = {};
    item.cbSize = sizeof(item);
    item.fMask = MIIM_BITMAP;
    item.hbmpItem = coffee;
    SetMenuItemInfoW(menu, IDM_COFFEE, FALSE, &item);
    // The check marks and the cup share one column at the left edge.
    MENUINFO info = {};
    info.cbSize = sizeof(info);
    info.fMask = MIM_STYLE;
    info.dwStyle = MNS_CHECKORBMP;
    SetMenuInfo(menu, &info);
}

HMENU BuildMenu(const Settings& settings, HBITMAP coffee)
{
    HMENU menu = CreatePopupMenu();
    HMENU levels = CreatePopupMenu();
    if (menu == nullptr || levels == nullptr) {
        if (menu != nullptr) {
            DestroyMenu(menu);
        }
        if (levels != nullptr) {
            DestroyMenu(levels);
        }
        return nullptr;
    }
    AppendMenuW(menu, MF_STRING, IDM_SETTINGS, L"Settings...");
    SetMenuDefaultItem(menu, IDM_SETTINGS, FALSE);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_EXT_TARGZ, L".tar.gz");
    AppendMenuW(menu, MF_STRING, IDM_EXT_TGZ, L".tgz");
    CheckMenuRadioItem(menu, IDM_EXT_TARGZ, IDM_EXT_TGZ,
                       settings.ext == ArchiveExt::Tgz ? IDM_EXT_TGZ : IDM_EXT_TARGZ, MF_BYCOMMAND);
    AppendMenuW(levels, MF_STRING, IDM_LEVEL_1, L"Fastest (1)");
    AppendMenuW(levels, MF_STRING, IDM_LEVEL_5, L"Normal (5)");
    AppendMenuW(levels, MF_STRING, IDM_LEVEL_9, L"Maximum (9)");
    CheckMenuRadioItem(levels, IDM_LEVEL_1, IDM_LEVEL_9, LevelCommand(settings.level), MF_BYCOMMAND);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(levels), L"Compression level");
    AppendMenuW(menu, MF_STRING | (settings.submenu ? MF_CHECKED : MF_UNCHECKED), IDM_SUBMENU, L"Explorer submenu");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AddCoffeeItem(menu, coffee);
    AppendMenuW(menu, MF_STRING, IDM_EXIT, L"Exit");
    return menu;
}

}  // namespace

TrayMenuResult ShowTrayMenu(HWND window, POINT point)
{
    Settings settings = ReadSettings();
    // DestroyMenu does not free item bitmaps. The DeleteObject calls below free this one.
    HBITMAP coffee = CreateCoffeeBitmap(MenuIconSize(point), GetSysColor(COLOR_HOTLIGHT));
    HMENU menu = BuildMenu(settings, coffee);
    if (menu == nullptr) {
        if (coffee != nullptr) {
            DeleteObject(coffee);
        }
        return TrayMenuResult::None;
    }
    // A tray menu without the foreground is not closed by a click elsewhere.
    SetForegroundWindow(window);
    const UINT align = GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0 ? TPM_RIGHTALIGN : TPM_LEFTALIGN;
    const UINT command = static_cast<UINT>(TrackPopupMenuEx(
        menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | align, point.x, point.y, window,
        nullptr));
    PostMessageW(window, WM_NULL, 0, 0);
    DestroyMenu(menu);
    if (coffee != nullptr) {
        DeleteObject(coffee);
    }
    switch (command) {
    case IDM_SETTINGS:
        return TrayMenuResult::OpenSettings;
    case IDM_COFFEE:
        return TrayMenuResult::BuyCoffee;
    case IDM_EXIT:
        return TrayMenuResult::Exit;
    case IDM_EXT_TARGZ:
        settings.ext = ArchiveExt::TarGz;
        break;
    case IDM_EXT_TGZ:
        settings.ext = ArchiveExt::Tgz;
        break;
    case IDM_LEVEL_1:
        settings.level = 1;
        break;
    case IDM_LEVEL_5:
        settings.level = 5;
        break;
    case IDM_LEVEL_9:
        settings.level = 9;
        break;
    case IDM_SUBMENU:
        settings.submenu = !settings.submenu;
        break;
    default:
        return TrayMenuResult::None;
    }
    std::wstring error;
    if (!WriteSettings(settings, error)) {
        MessageBoxW(window, error.c_str(), L"AddToTarGz", MB_OK | MB_ICONERROR);
        return TrayMenuResult::None;
    }
    return TrayMenuResult::SettingsChanged;
}
