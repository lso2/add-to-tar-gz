// tray_icon.cpp: Shell_NotifyIconW calls for the AddToTarGz icon.
// Globals: none beyond tray_icon.h.
// Doc: docs/notes/src/tray/tray_icon.md
#include "tray_icon.h"
#include "resource_ids.h"
#include "tray_window.h"
#include <commctrl.h>
#include <shellapi.h>

namespace {

constexpr UINT kIconId = 1;
HICON g_icon = nullptr;

NOTIFYICONDATAW IconData(HWND window)
{
    NOTIFYICONDATAW data = {};
    data.cbSize = sizeof(data);
    data.hWnd = window;
    data.uID = kIconId;
    return data;
}

}  // namespace

bool AddTrayIcon(HWND window, HINSTANCE instance)
{
    if (g_icon == nullptr) {
        // LoadIconMetric picks the image for the notification area's size at the current DPI.
        LoadIconMetric(instance, MAKEINTRESOURCEW(IDI_TGZ), LIM_SMALL, &g_icon);
    }
    NOTIFYICONDATAW data = IconData(window);
    data.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
    data.uCallbackMessage = WM_ATG_TRAY;
    data.hIcon = g_icon;
    wcscpy_s(data.szTip, L"AddToTarGz");
    if (!Shell_NotifyIconW(NIM_ADD, &data)) {
        return false;
    }
    data.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &data);
    return true;
}

void RemoveTrayIcon(HWND window)
{
    NOTIFYICONDATAW data = IconData(window);
    Shell_NotifyIconW(NIM_DELETE, &data);
}
