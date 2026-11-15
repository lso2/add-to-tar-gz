// release_version.h: version numbers of AddToTarGz releases, read from GitHub release tags.
// Globals: ReleaseVersion, ParseReleaseTag, CompareVersions, VersionText, CurrentVersion.
// Doc: docs/notes/src/tray/release_version.md
#pragma once
#include <array>
#include <string>

// Major, minor, patch and revision. A tag with fewer numbers has zeros for the rest.
using ReleaseVersion = std::array<unsigned, 4>;

// The first run of 2 to 4 dot-separated numbers in the tag, the rule of QuickPane's TryParseTag:
// "v2.1.0" and "AddToTarGzSetup-2.1.0-release" are both 2.1.0. False when the tag has no such run
// or a number in it is above 2147483647.
bool ParseReleaseTag(const std::wstring& tag, ReleaseVersion& version);

// Below, equal to or above zero as `a` is older than, the same as or newer than `b`.
int CompareVersions(const ReleaseVersion& a, const ReleaseVersion& b);

// "2.1.0", with the revision added only when it is not zero.
std::wstring VersionText(const ReleaseVersion& version);

// This build's version from version.h.
ReleaseVersion CurrentVersion();
