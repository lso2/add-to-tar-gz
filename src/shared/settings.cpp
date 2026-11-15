// settings.cpp: reads and writes HKCU\Software\AddToTarGz.
// Globals: none beyond settings.h.
// Doc: docs/notes/src/shared/settings.md
#include "settings.h"
#include "errors.h"
#include <windows.h>

namespace {

constexpr wchar_t kSettingsKey[] = L"Software\\AddToTarGz";

struct FlagValue {
    const wchar_t* name;
    bool Settings::*member;
};

// Every on/off value, in the order the settings window lists them.
constexpr FlagValue kFlags[] = {
    {L"Submenu", &Settings::submenu},
    {L"MenuIcons", &Settings::menuIcons},
    {L"ItemAdd", &Settings::itemAdd},
    {L"ItemAddOther", &Settings::itemAddOther},
    {L"ItemExtractHere", &Settings::itemExtractHere},
    {L"ItemExtractFolder", &Settings::itemExtractFolder},
    {L"ItemExtractTo", &Settings::itemExtractTo},
    {L"ItemSettings", &Settings::itemSettings},
    {L"TrayIcon", &Settings::trayIcon},
};

bool ReadDword(const wchar_t* name, DWORD& value)
{
    DWORD bytes = sizeof(value);
    return RegGetValueW(HKEY_CURRENT_USER, kSettingsKey, name, RRF_RT_REG_DWORD, nullptr, &value, &bytes) ==
           ERROR_SUCCESS;
}

}  // namespace

Settings ReadSettings()
{
    Settings settings;
    wchar_t extension[16] = {};
    DWORD bytes = sizeof(extension);
    if (RegGetValueW(HKEY_CURRENT_USER, kSettingsKey, L"Extension", RRF_RT_REG_SZ, nullptr, extension, &bytes) ==
            ERROR_SUCCESS &&
        wcscmp(extension, L"tgz") == 0) {
        settings.ext = ArchiveExt::Tgz;
    }
    DWORD level = 0;
    if (ReadDword(L"CompressionLevel", level) && (level == 1 || level == 5 || level == 9)) {
        settings.level = static_cast<int>(level);
    }
    for (const FlagValue& flag : kFlags) {
        DWORD value = 0;
        if (ReadDword(flag.name, value)) {
            settings.*flag.member = value != 0;
        }
    }
    return settings;
}

bool WriteSettings(const Settings& settings, std::wstring& error)
{
    HKEY key = nullptr;
    LSTATUS status =
        RegCreateKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
    if (status != ERROR_SUCCESS) {
        error = L"Opening HKCU\\Software\\AddToTarGz failed: " + WindowsErrorText(static_cast<DWORD>(status));
        return false;
    }
    const wchar_t* failed = nullptr;
    const wchar_t* extension = settings.ext == ArchiveExt::Tgz ? L"tgz" : L"tar.gz";
    status = RegSetValueExW(key, L"Extension", 0, REG_SZ, reinterpret_cast<const BYTE*>(extension),
                            static_cast<DWORD>((wcslen(extension) + 1) * sizeof(wchar_t)));
    if (status != ERROR_SUCCESS) {
        failed = L"Extension";
    }
    if (failed == nullptr) {
        const DWORD level = static_cast<DWORD>(settings.level);
        status = RegSetValueExW(key, L"CompressionLevel", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&level),
                                sizeof(level));
        if (status != ERROR_SUCCESS) {
            failed = L"CompressionLevel";
        }
    }
    for (const FlagValue& flag : kFlags) {
        if (failed != nullptr) {
            break;
        }
        const DWORD value = (settings.*flag.member) ? 1 : 0;
        status = RegSetValueExW(key, flag.name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
        if (status != ERROR_SUCCESS) {
            failed = flag.name;
        }
    }
    RegCloseKey(key);
    if (failed != nullptr) {
        error = L"Writing " + std::wstring(failed) + L" under HKCU\\Software\\AddToTarGz failed: " +
                WindowsErrorText(static_cast<DWORD>(status));
        return false;
    }
    return true;
}
