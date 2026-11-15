// settings_footer.h: the footer of the Settings window, after QuickPane's settings footer: version, update status,
// Download button, GitHub link and Buy me a coffee button.
// Globals: InitFooter, ShowUpdateResult, OnFooterCommand, OnFooterDrawItem, UpdateFooterDpi, CloseFooter.
// Doc: docs/notes/src/tray/settings_footer.md
#pragma once
#include <windows.h>

struct UpdateResult;

// Sets the version, "Checking for updates...", the GitHub link text, the cup image and the tooltip control.
void InitFooter(HWND dialog);

// Shows a finished check: "Up to date", "Version X is available" with the Download button, or
// "Update check failed" with the reason in the status tooltip.
void ShowUpdateResult(HWND dialog, const UpdateResult& result);

// Opens the download, the donate page or the repository for a click on Download, Buy me a coffee or the GitHub
// link. False for other commands.
bool OnFooterCommand(HWND dialog, WORD id);

// Draws the GitHub link: GitHub's mark and the text "GitHub" in the text color, or in the link color while the
// pointer is over them. False for other controls.
bool OnFooterDrawItem(const DRAWITEMSTRUCT& item);

// Draws the cup again for the dialog's new DPI, from WM_DPICHANGED.
void UpdateFooterDpi(HWND dialog, UINT dpi);

// Frees the cup image, from WM_DESTROY.
void CloseFooter(HWND dialog);
