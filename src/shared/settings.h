// settings.h: per-user options under HKCU\Software\AddToTarGz. The menu DLL and the tray app read them, and the
// tray app's settings window and tray menu write them. Installer.iss creates the key and deletes it at uninstall.
// Globals: Settings, ReadSettings, WriteSettings.
// Doc: docs/notes/src/shared/settings.md
#pragma once
#include <string>
#include "names.h"

struct Settings {
    ArchiveExt ext = ArchiveExt::TarGz;  // "Extension": "tar.gz" or "tgz"
    int level = 5;                       // "CompressionLevel": gzip level 1, 5 or 9
    bool submenu = true;                 // "Submenu": every item inside one AddToTarGz submenu
    bool menuIcons = true;               // "MenuIcons": the tgz icon beside the menu entries
    bool itemAdd = true;                 // "ItemAdd": Add to "name.<extension>"
    bool itemAddOther = false;           // "ItemAddOther": Add to "name.<other extension>"
    bool itemExtractHere = true;         // "ItemExtractHere": Extract here
    bool itemExtractFolder = true;       // "ItemExtractFolder": Extract to "name\"
    bool itemExtractTo = true;           // "ItemExtractTo": Extract to... with a folder dialog
    bool itemSettings = true;            // "ItemSettings": Settings... in the Explorer menu
    bool trayIcon = true;                // "TrayIcon": tray icon shown and started at sign-in
};

// A missing value means the default above. A level other than 1, 5 or 9 reads as 5.
Settings ReadSettings();

// Writes every value. Returns false with the Windows error text when a write fails.
bool WriteSettings(const Settings& settings, std::wstring& error);
