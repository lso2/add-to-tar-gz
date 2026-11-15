// tray_window.cpp: window procedure of the hidden tray window: icon events, the tray menu, the settings window
// with its update check, and closing for the installer and for sign-out.
// Globals: none beyond tray_window.h.
// Doc: docs/notes/src/tray/tray_window.md
#include "tray_window.h"
#include "errors.h"
#include "links.h"
#include "resource_ids.h"
#include "settings.h"
#include "settings_dialog.h"
#include "tray_icon.h"
#include "tray_menu.h"
#include "update_check.h"
#include "update_result.h"
#include "web_link.h"
#include <shellapi.h>
#include <memory>
#include <string>
#include <windowsx.h>

namespace {

struct TrayState {
    HINSTANCE instance = nullptr;
    HWND dialog = nullptr;
    bool iconShown = false;
    UINT taskbarCreated = 0;
    bool checking = false;  // an update check thread is running
};

TrayState g_state;

// Shows or hides the icon to match the TrayIcon setting. Returns the setting.
bool SyncIcon(HWND window)
{
    const bool wanted = ReadSettings().trayIcon;
    if (wanted && !g_state.iconShown) {
        g_state.iconShown = AddTrayIcon(window, g_state.instance);
    } else if (!wanted && g_state.iconShown) {
        RemoveTrayIcon(window);
        g_state.iconShown = false;
    }
    return wanted;
}

// Opening a settings window starts a check, as QuickPane does each time it builds its settings footer.
// A window opened during a running check shows that check's result.
void CheckForUpdate(HWND window)
{
    if (g_state.checking) {
        return;
    }
    std::wstring error;
    g_state.checking = StartUpdateCheck(window, WM_ATG_UPDATE_RESULT, error);
    if (!g_state.checking) {
        UpdateResult failed;
        failed.error = error;
        SendMessageW(g_state.dialog, WM_ATG_UPDATE_RESULT, 0, reinterpret_cast<LPARAM>(&failed));
    }
}

void ShowSettings(HWND window)
{
    if (g_state.dialog != nullptr) {
        ShowWindow(g_state.dialog, SW_RESTORE);
        SetForegroundWindow(g_state.dialog);
        return;
    }
    g_state.dialog = OpenSettingsDialog(g_state.instance, window);
    if (g_state.dialog == nullptr) {
        const std::wstring text = L"The settings window could not be created: " + WindowsErrorText(GetLastError());
        MessageBoxW(nullptr, text.c_str(), L"AddToTarGz", MB_OK | MB_ICONERROR);
        return;
    }
    CheckForUpdate(window);
}

void Quit(HWND window)
{
    if (g_state.dialog != nullptr) {
        DestroyWindow(g_state.dialog);
        g_state.dialog = nullptr;
    }
    if (g_state.iconShown) {
        RemoveTrayIcon(window);
        g_state.iconShown = false;
    }
    DestroyWindow(window);
}

void OnTrayEvent(HWND window, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(lParam)) {
    case WM_CONTEXTMENU: {
        const POINT point = {GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam)};
        switch (ShowTrayMenu(window, point)) {
        case TrayMenuResult::OpenSettings:
            ShowSettings(window);
            break;
        case TrayMenuResult::SettingsChanged:
            if (g_state.dialog != nullptr) {
                SendMessageW(g_state.dialog, WM_ATG_RELOAD, 0, 0);
            }
            break;
        case TrayMenuResult::BuyCoffee:
            OpenWebPage(window, kDonateUrl);
            break;
        case TrayMenuResult::Exit:
            Quit(window);
            break;
        default:
            break;
        }
        break;
    }
    case NIN_SELECT:
    case NIN_KEYSELECT:
        ShowSettings(window);
        break;
    default:
        break;
    }
}

LRESULT CALLBACK TrayWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (g_state.taskbarCreated != 0 && message == g_state.taskbarCreated) {
        // A restarted Explorer has an empty notification area. NIM_ADD fails for an icon already in the taskbar,
        // and deleting first covers a TaskbarCreated message posted while the old icon is still there.
        RemoveTrayIcon(window);
        g_state.iconShown = false;
        SyncIcon(window);
        return 0;
    }
    switch (message) {
    case WM_ATG_TRAY:
        OnTrayEvent(window, wParam, lParam);
        return 0;
    case WM_ATG_SHOW_SETTINGS:
        ShowSettings(window);
        return 0;
    case WM_ATG_SETTINGS_APPLIED:
        SyncIcon(window);
        return 0;
    case WM_ATG_UPDATE_RESULT: {
        std::unique_ptr<UpdateResult> result(reinterpret_cast<UpdateResult*>(lParam));
        g_state.checking = false;
        if (g_state.dialog != nullptr) {
            SendMessageW(g_state.dialog, WM_ATG_UPDATE_RESULT, 0, lParam);
        }
        return 0;
    }
    case WM_ATG_SETTINGS_CLOSED:
        g_state.dialog = nullptr;
        // With the tray icon off, the app runs only while the settings window is open.
        if (!SyncIcon(window)) {
            Quit(window);
        }
        return 0;
    case WM_CLOSE:
        Quit(window);
        return 0;
    case WM_QUERYENDSESSION:
        return TRUE;
    case WM_ENDSESSION:
        if (wParam != FALSE) {
            Quit(window);
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

}  // namespace

HWND CreateTrayWindow(HINSTANCE instance, bool openSettings)
{
    g_state.instance = instance;
    g_state.taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = TrayWindowProc;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_TGZ));
    windowClass.lpszClassName = kTrayWindowClass;
    if (RegisterClassExW(&windowClass) == 0) {
        return nullptr;
    }
    // Top-level and never shown: broadcasts such as TaskbarCreated and WM_QUERYENDSESSION are sent to top-level
    // windows only.
    HWND window = CreateWindowExW(WS_EX_TOOLWINDOW, kTrayWindowClass, L"AddToTarGz", WS_POPUP, 0, 0, 0, 0, nullptr,
                                  nullptr, instance, nullptr);
    if (window == nullptr) {
        return nullptr;
    }
    const bool trayOn = SyncIcon(window);
    if (openSettings) {
        ShowSettings(window);
    }
    // With the tray icon off and no settings window, the posted WM_CLOSE is the last message of the loop.
    if (!trayOn && g_state.dialog == nullptr) {
        PostMessageW(window, WM_CLOSE, 0, 0);
    }
    return window;
}

HWND TrayDialog()
{
    return g_state.dialog;
}
