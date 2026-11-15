// github_release.h: reads GitHub API answers about AddToTarGz releases.
// Globals: ReadLatestRelease, ReadErrorMessage.
// Doc: docs/notes/src/tray/github_release.md
#pragma once
#include "update_result.h"
#include <string>

// Reads a "latest release" answer (UTF-8 JSON) and compares its tag with this build's version.
UpdateResult ReadLatestRelease(const std::string& json);

// The "message" text of a GitHub error answer, or an empty string when the answer has none.
std::wstring ReadErrorMessage(const std::string& json);
