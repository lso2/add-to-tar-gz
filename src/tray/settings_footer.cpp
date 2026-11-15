// settings_footer.cpp: footer controls of the Settings window and the update result they show.
// Globals: none beyond settings_footer.h.
// Doc: docs/notes/src/tray/settings_footer.md
#include "settings_footer.h"
#include "coffee_icon.h"
#include "github_mark.h"
#include "links.h"
#include "tray_ids.h"
#include "update_result.h"
#include "version.h"
#include "web_link.h"
#include <commctrl.h>
#include <string>

namespace {

// tray_window.cpp opens at most one Settings window. This state is the footer of that window.
struct FooterState {
    HICON coffee = nullptr;
    HICON mark = nullptr;     // GitHub's mark in the text color
    HICON markHot = nullptr;  // GitHub's mark in the link color, for hover
    int markSize = 0;         // 0 when the build has no mark
    bool hot = false;         // the pointer is over the GitHub link
    HWND tooltip = nullptr;
    std::wstring downloadUrl;
};

FooterState g_footer;

// Widest tooltip before its text wraps, in pixels at 96 DPI.
constexpr int kTipWidth = 360;
// Space between GitHub's mark and the link text, in pixels at 96 DPI.
constexpr int kMarkGap = 4;

void SetCoffeeImage(HWND dialog, UINT dpi)
{
    HICON icon = CreateCoffeeIcon(GetSystemMetricsForDpi(SM_CXSMICON, dpi), GetSysColor(COLOR_HOTLIGHT));
    if (icon == nullptr) {
        return;
    }
    SendDlgItemMessageW(dialog, IDC_COFFEE, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(icon));
    if (g_footer.coffee != nullptr) {
        DestroyIcon(g_footer.coffee);
    }
    g_footer.coffee = icon;
}

int MarkGap(HWND link)
{
    return g_footer.markSize > 0 ? MulDiv(kMarkGap, static_cast<int>(GetDpiForWindow(link)), 96) : 0;
}

// The marks for this DPI, and the link control sized to the mark and the text. The hand pointer and the hover
// color then cover the drawn link only.
void SetGithubLink(HWND dialog, UINT dpi)
{
    const int pixels = GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(dialog, GWLP_HINSTANCE));
    if (g_footer.mark != nullptr) {
        DestroyIcon(g_footer.mark);
    }
    if (g_footer.markHot != nullptr) {
        DestroyIcon(g_footer.markHot);
    }
    g_footer.mark = LoadGithubMark(instance, pixels, GetSysColor(COLOR_BTNTEXT));
    g_footer.markHot = LoadGithubMark(instance, pixels, GetSysColor(COLOR_HOTLIGHT));
    g_footer.markSize = g_footer.mark != nullptr && g_footer.markHot != nullptr ? pixels : 0;
    HWND link = GetDlgItem(dialog, IDC_GITHUB);
    wchar_t text[64] = {};
    GetWindowTextW(link, text, 64);
    HDC dc = GetDC(link);
    HGDIOBJ oldFont = SelectObject(dc, reinterpret_cast<HFONT>(SendMessageW(link, WM_GETFONT, 0, 0)));
    SIZE extent = {};
    GetTextExtentPoint32W(dc, text, lstrlenW(text), &extent);
    SelectObject(dc, oldFont);
    ReleaseDC(link, dc);
    RECT rect = {};
    GetWindowRect(link, &rect);
    // 2 pixels more for the focus rectangle around the text.
    SetWindowPos(link, nullptr, 0, 0, g_footer.markSize + MarkGap(link) + extent.cx + 2, rect.bottom - rect.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    InvalidateRect(link, nullptr, FALSE);
}

// Hover, the hand pointer and Enter for the owner-drawn GitHub link.
LRESULT CALLBACK GithubLinkProc(HWND link, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
{
    switch (message) {
    case WM_MOUSEMOVE:
        if (!g_footer.hot) {
            g_footer.hot = true;
            TRACKMOUSEEVENT track = {};
            track.cbSize = sizeof(track);
            track.dwFlags = TME_LEAVE;
            track.hwndTrack = link;
            TrackMouseEvent(&track);
            InvalidateRect(link, nullptr, FALSE);
        }
        break;
    case WM_MOUSELEAVE:
        g_footer.hot = false;
        InvalidateRect(link, nullptr, FALSE);
        break;
    case WM_SETCURSOR:
        SetCursor(LoadCursorW(nullptr, IDC_HAND));
        return TRUE;
    case WM_GETDLGCODE:
        // Without DLGC_WANTMESSAGE for Enter, the dialog manager sends Enter to the OK button.
        if (lParam != 0) {
            const MSG* key = reinterpret_cast<const MSG*>(lParam);
            if (key->message == WM_KEYDOWN && key->wParam == VK_RETURN) {
                return DLGC_BUTTON | DLGC_WANTMESSAGE;
            }
        }
        return DLGC_BUTTON;
    case WM_KEYDOWN:
        if (wParam == VK_RETURN) {
            SendMessageW(GetParent(link), WM_COMMAND, MAKEWPARAM(IDC_GITHUB, BN_CLICKED), reinterpret_cast<LPARAM>(link));
            return 0;
        }
        break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(link, GithubLinkProc, id);
        break;
    default:
        break;
    }
    return DefSubclassProc(link, message, wParam, lParam);
}

HWND CreateTooltip(HWND dialog, UINT dpi)
{
    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(dialog, GWLP_HINSTANCE));
    HWND tooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                                   CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, dialog, nullptr,
                                   instance, nullptr);
    if (tooltip != nullptr) {
        SendMessageW(tooltip, TTM_SETMAXTIPWIDTH, 0, MulDiv(kTipWidth, static_cast<int>(dpi), 96));
    }
    return tooltip;
}

// Replaces the control's tooltip. An empty text leaves the control with no tooltip.
void SetTip(HWND dialog, int id, const std::wstring& text)
{
    if (g_footer.tooltip == nullptr) {
        return;
    }
    TTTOOLINFOW tool = {};
    tool.cbSize = sizeof(tool);
    tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    tool.hwnd = dialog;
    tool.uId = reinterpret_cast<UINT_PTR>(GetDlgItem(dialog, id));
    SendMessageW(g_footer.tooltip, TTM_DELTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
    if (!text.empty()) {
        // TTM_ADDTOOLW copies the text into the tooltip control.
        tool.lpszText = const_cast<wchar_t*>(text.c_str());
        SendMessageW(g_footer.tooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
    }
}

}  // namespace

void InitFooter(HWND dialog)
{
    g_footer = FooterState();
    const UINT dpi = GetDpiForWindow(dialog);
    SetDlgItemTextW(dialog, IDC_VERSION, L"AddToTarGz " ATG_VERSION_STRING);
    SetDlgItemTextW(dialog, IDC_UPDATE_STATUS, L"Checking for updates...");
    SetWindowSubclass(GetDlgItem(dialog, IDC_GITHUB), GithubLinkProc, 0, 0);
    g_footer.tooltip = CreateTooltip(dialog, dpi);
    SetCoffeeImage(dialog, dpi);
    SetGithubLink(dialog, dpi);
}

void ShowUpdateResult(HWND dialog, const UpdateResult& result)
{
    HWND download = GetDlgItem(dialog, IDC_DOWNLOAD);
    if (!result.error.empty() || result.latest.empty()) {
        SetDlgItemTextW(dialog, IDC_UPDATE_STATUS, L"Update check failed");
        SetTip(dialog, IDC_UPDATE_STATUS, result.error);
        ShowWindow(download, SW_HIDE);
        return;
    }
    SetTip(dialog, IDC_UPDATE_STATUS, std::wstring());
    if (!result.available) {
        SetDlgItemTextW(dialog, IDC_UPDATE_STATUS, L"Up to date");
        ShowWindow(download, SW_HIDE);
        return;
    }
    SetDlgItemTextW(dialog, IDC_UPDATE_STATUS, (L"Version " + result.latest + L" is available").c_str());
    SetWindowTextW(download, (L"Download " + result.latest).c_str());
    g_footer.downloadUrl = result.downloadUrl;
    SetTip(dialog, IDC_DOWNLOAD, result.downloadUrl);
    ShowWindow(download, SW_SHOW);
}

bool OnFooterCommand(HWND dialog, WORD id)
{
    switch (id) {
    case IDC_DOWNLOAD:
        if (!g_footer.downloadUrl.empty()) {
            OpenWebPage(dialog, g_footer.downloadUrl.c_str());
        }
        return true;
    case IDC_COFFEE:
        OpenWebPage(dialog, kDonateUrl);
        return true;
    case IDC_GITHUB:
        OpenWebPage(dialog, kRepoUrl);
        return true;
    default:
        return false;
    }
}

// The link of the Claude Folders organizer footer: GitHub's mark and the text in the dialog's text color, both in
// the link color while the pointer is over them, never underlined.
bool OnFooterDrawItem(const DRAWITEMSTRUCT& item)
{
    if (item.CtlType != ODT_BUTTON || item.CtlID != IDC_GITHUB) {
        return false;
    }
    const bool hot = g_footer.hot;
    FillRect(item.hDC, &item.rcItem, GetSysColorBrush(COLOR_BTNFACE));
    RECT text = item.rcItem;
    text.left += 1;
    if (g_footer.markSize > 0) {
        const int top = item.rcItem.top + (item.rcItem.bottom - item.rcItem.top - g_footer.markSize) / 2;
        DrawIconEx(item.hDC, text.left, top, hot ? g_footer.markHot : g_footer.mark, g_footer.markSize, g_footer.markSize,
                   0, nullptr, DI_NORMAL);
        text.left += g_footer.markSize + MarkGap(item.hwndItem);
    }
    wchar_t caption[64] = {};
    GetWindowTextW(item.hwndItem, caption, 64);
    HGDIOBJ oldFont = SelectObject(item.hDC, reinterpret_cast<HFONT>(SendMessageW(item.hwndItem, WM_GETFONT, 0, 0)));
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, GetSysColor(hot ? COLOR_HOTLIGHT : COLOR_BTNTEXT));
    DrawTextW(item.hDC, caption, -1, &text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    if ((item.itemState & ODS_FOCUS) != 0 && (item.itemState & ODS_NOFOCUSRECT) == 0) {
        RECT focus = text;
        DrawTextW(item.hDC, caption, -1, &focus, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
        const int height = focus.bottom - focus.top;
        focus.top = text.top + (text.bottom - text.top - height) / 2;
        focus.bottom = focus.top + height;
        InflateRect(&focus, 1, 0);
        DrawFocusRect(item.hDC, &focus);
    }
    SelectObject(item.hDC, oldFont);
    return true;
}

void UpdateFooterDpi(HWND dialog, UINT dpi)
{
    SetCoffeeImage(dialog, dpi);
    SetGithubLink(dialog, dpi);
    if (g_footer.tooltip != nullptr) {
        SendMessageW(g_footer.tooltip, TTM_SETMAXTIPWIDTH, 0, MulDiv(kTipWidth, static_cast<int>(dpi), 96));
    }
}

void CloseFooter(HWND dialog)
{
    SendDlgItemMessageW(dialog, IDC_COFFEE, BM_SETIMAGE, IMAGE_ICON, 0);
    if (g_footer.coffee != nullptr) {
        DestroyIcon(g_footer.coffee);
    }
    if (g_footer.mark != nullptr) {
        DestroyIcon(g_footer.mark);
    }
    if (g_footer.markHot != nullptr) {
        DestroyIcon(g_footer.markHot);
    }
    // The tooltip control is owned by the dialog, and DestroyWindow on the dialog destroys it.
    g_footer = FooterState();
}
