// update_result.h: the outcome of one update check, passed from the check's thread to the settings window.
// Globals: UpdateResult.
// Doc: docs/notes/src/tray/update_result.md
#pragma once
#include <string>

struct UpdateResult {
    bool available = false;    // the release is newer than this build
    std::wstring latest;       // the release's version as VersionText writes it, empty when the check failed
    std::wstring downloadUrl;  // the .exe asset, else the .zip asset, else the release page
    std::wstring error;        // the reason the check failed
};
