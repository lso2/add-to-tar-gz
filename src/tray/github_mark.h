// github_mark.h: GitHub's mark for the settings footer, decoded from the PNG resource that the build embeds when
// icon\github-mark.png exists.
// Globals: CreateIconFromPng, LoadGithubMark.
// Doc: docs/notes/src/tray/github_mark.md
#pragma once
#include <windows.h>

// Decodes PNG bytes with WIC into a pixels x pixels icon, the image scaled with its proportions kept and centered.
// Every pixel gets `color` and keeps the PNG's alpha, as the Claude Folders footer draws the mark in the link's
// color. nullptr when WIC fails. The calling thread has COM initialized.
HICON CreateIconFromPng(const void* data, DWORD size, int pixels, COLORREF color);

// The IDR_GITHUB_MARK resource as an icon. nullptr when the build has no icon\github-mark.png.
HICON LoadGithubMark(HINSTANCE instance, int pixels, COLORREF color);
