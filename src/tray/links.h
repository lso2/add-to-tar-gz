// links.h: web addresses of the AddToTarGz project that the tray app requests or opens.
// Globals: kRepoUrl, kLatestReleaseApi, kReleasesPage, kDonateUrl.
// Doc: docs/notes/src/tray/links.md
#pragma once

// The source repository, opened from the GitHub link in the settings window.
inline constexpr wchar_t kRepoUrl[] = L"https://github.com/lso2/add-to-tar-gz";

// GitHub's newest published release (drafts and prereleases excluded), requested by the update check.
inline constexpr wchar_t kLatestReleaseApi[] = L"https://api.github.com/repos/lso2/add-to-tar-gz/releases/latest";

// The Download target for a release with no usable asset or page address.
inline constexpr wchar_t kReleasesPage[] = L"https://github.com/lso2/add-to-tar-gz/releases/latest";

// The page behind Buy me a coffee in the settings window and the tray menu.
inline constexpr wchar_t kDonateUrl[] = L"https://plexpixel.com/donate";
