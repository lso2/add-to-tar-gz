// folder_picker.h: the "Extract to" folder dialog that --pick-dest shows before the jobs start.
// Globals: PickFolder.
// Doc: docs/notes/src/worker/folder_picker.md
#pragma once
#include <windows.h>
#include <string>

// Shows the Windows folder dialog opened at startFolder. COM must be initialized on the calling thread.
// Returns S_OK with `picked` set, HRESULT_FROM_WIN32(ERROR_CANCELLED) when the dialog is closed without a folder,
// or the HRESULT of the failing call.
HRESULT PickFolder(const std::wstring& startFolder, std::wstring& picked);
