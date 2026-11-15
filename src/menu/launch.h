// launch.h: starts AddToTarGz.exe or AddToTarGzTray.exe from the DLL's own folder for one menu command.
// Globals: LaunchCommand.
// Doc: docs/notes/src/menu/launch.md
#pragma once
#include <windows.h>
#include <vector>
#include "menu_entries.h"
#include "selection.h"
#include "settings.h"

// Archive commands start one worker process per batch of paths under the 32767-character command line limit.
// The one-archive command starts one worker and hands it every path in shared memory. Settings... starts AddToTarGzTray.exe --settings. A dialog owned by `owner` shows the Windows error when an exe
// does not start.
HRESULT LaunchCommand(MenuCommand command, const Settings& settings, const std::vector<SelectedItem>& items,
                      HWND owner);
