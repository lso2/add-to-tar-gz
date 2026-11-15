// autostart.cpp: writes or deletes the AddToTarGz Run value.
// Globals: none beyond autostart.h.
// Doc: docs/notes/src/tray/autostart.md
#include "autostart.h"
#include "errors.h"
#include "paths.h"
#include <windows.h>

namespace {

constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kRunValue[] = L"AddToTarGz";

}  // namespace

bool SetStartAtSignIn(bool enabled, std::wstring& error)
{
    if (!enabled) {
        const LSTATUS status = RegDeleteKeyValueW(HKEY_CURRENT_USER, kRunKey, kRunValue);
        if (status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND) {
            return true;
        }
        error = L"Deleting the AddToTarGz value under HKCU\\" + std::wstring(kRunKey) + L" failed: " +
                WindowsErrorText(static_cast<DWORD>(status));
        return false;
    }
    // The same text Installer.iss writes: the quoted exe path, then --tray.
    const std::wstring command = L"\"" + ModulePath(nullptr) + L"\" --tray";
    const LSTATUS status =
        RegSetKeyValueW(HKEY_CURRENT_USER, kRunKey, kRunValue, REG_SZ, command.c_str(),
                        static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    if (status != ERROR_SUCCESS) {
        error = L"Writing the AddToTarGz value under HKCU\\" + std::wstring(kRunKey) + L" failed: " +
                WindowsErrorText(static_cast<DWORD>(status));
        return false;
    }
    return true;
}
