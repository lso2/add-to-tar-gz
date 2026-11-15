// main.cpp: entry point of AddToTarGzTray.exe [--tray | --settings]. Allows one running instance per session, then
// runs the tray window's message loop. --tray starts at sign-in with the icon only, and --settings or no option opens
// the settings window.
// Globals: wWinMain.
// Doc: docs/notes/src/tray/main.md
#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <shellapi.h>
#include <string>
#include "errors.h"
#include "old_builds.h"
#include "settings.h"
#include "tray_window.h"

namespace {

bool HasOption(const wchar_t* option)
{
    int count = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &count);
    if (argv == nullptr) {
        return false;
    }
    bool found = false;
    for (int i = 1; i < count; ++i) {
        found = found || wcscmp(argv[i], option) == 0;
    }
    LocalFree(argv);
    return found;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    const bool trayOnly = HasOption(L"--tray");
    HANDLE instanceMutex = CreateMutexW(nullptr, FALSE, L"Local\\AddToTarGzTray");
    if (instanceMutex != nullptr && GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND running = FindWindowW(kTrayWindowClass, nullptr);
        if (running != nullptr && !trayOnly) {
            DWORD process = 0;
            GetWindowThreadProcessId(running, &process);
            // The running instance shows its settings window in front with this process's foreground right.
            AllowSetForegroundWindow(process);
            PostMessageW(running, WM_ATG_SHOW_SETTINGS, 0, 0);
        }
        CloseHandle(instanceMutex);
        return 0;
    }
    RemoveOldBuilds();
    if (trayOnly && !ReadSettings().trayIcon) {
        if (instanceMutex != nullptr) {
            CloseHandle(instanceMutex);
        }
        return 0;
    }
    INITCOMMONCONTROLSEX controls = {sizeof(controls), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    // ShellExecuteExW requires COM on the calling thread when it loads a shell extension.
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    // The Restart Manager of another installer or of Windows starts the tray app again with --tray after closing it.
    RegisterApplicationRestart(L"--tray", RESTART_NO_CRASH | RESTART_NO_HANG | RESTART_NO_REBOOT);

    HWND window = CreateTrayWindow(instance, !trayOnly);
    if (window == nullptr) {
        const std::wstring text = L"AddToTarGzTray.exe could not create its window: " + WindowsErrorText(GetLastError());
        MessageBoxW(nullptr, text.c_str(), L"AddToTarGz", MB_OK | MB_ICONERROR);
        if (SUCCEEDED(com)) {
            CoUninitialize();
        }
        if (instanceMutex != nullptr) {
            CloseHandle(instanceMutex);
        }
        return 1;
    }
    MSG message = {};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        HWND dialog = TrayDialog();
        if (dialog != nullptr && IsDialogMessageW(dialog, &message)) {
            continue;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if (SUCCEEDED(com)) {
        CoUninitialize();
    }
    if (instanceMutex != nullptr) {
        CloseHandle(instanceMutex);
    }
    return 0;
}
