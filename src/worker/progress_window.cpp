// progress_window.cpp: Win32 progress window for running jobs.
// Globals: none beyond progress_window.h.
// Doc: docs/notes/src/worker/progress_window.md
#include "progress_window.h"
#include "resource_ids.h"
#include <windows.h>
#include <commctrl.h>
#include <string>

namespace {

constexpr UINT_PTR kTimerId = 1;
constexpr int kLineId = 100;
constexpr int kCountId = 101;
constexpr int kBarId = 102;
constexpr wchar_t kClassName[] = L"AddToTarGzProgress";

struct WindowState {
    Progress* progress = nullptr;
    HFONT font = nullptr;
    int dpi = 96;
    ULONGLONG startTick = 0;
    bool shown = false;
    bool cancelRequested = false;
    std::wstring lastLine;
    std::wstring lastCount;
};

int Scale(int value, int dpi)
{
    return MulDiv(value, dpi, 96);
}

std::wstring FormatBytes(uint64_t bytes)
{
    const wchar_t* units[] = {L"bytes", L"KB", L"MB", L"GB", L"TB"};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    wchar_t text[64] = {};
    if (unit == 0) {
        swprintf_s(text, L"%llu %s", static_cast<unsigned long long>(bytes), units[0]);
    } else {
        swprintf_s(text, L"%.1f %s", value, units[unit]);
    }
    return text;
}

void SetTextIfChanged(HWND window, int id, const std::wstring& text, std::wstring& last)
{
    if (text != last) {
        SetDlgItemTextW(window, id, text.c_str());
        last = text;
    }
}

void RequestCancel(HWND window, WindowState& state)
{
    if (state.cancelRequested) {
        return;
    }
    state.cancelRequested = true;
    state.progress->cancel = true;
    EnableWindow(GetDlgItem(window, IDCANCEL), FALSE);
    SetTextIfChanged(window, kLineId, L"Canceling", state.lastLine);
}

void Refresh(HWND window, WindowState& state)
{
    if (state.progress->finished.load()) {
        DestroyWindow(window);
        return;
    }
    const Progress::View view = state.progress->Snapshot();
    const uint64_t done = state.progress->doneBytes.load();
    if (!state.cancelRequested) {
        SetTextIfChanged(window, kLineId, view.line, state.lastLine);
    }
    std::wstring count;
    if (view.count > 1) {
        count = L"Item " + std::to_wstring(view.index + 1) + L" of " + std::to_wstring(view.count) + L", ";
    }
    count += FormatBytes(done) + L" of " + FormatBytes(view.totalBytes);
    SetTextIfChanged(window, kCountId, count, state.lastCount);
    // Tar headers add to the byte count beyond the file total. The bar stops at 990 of 1000.
    const uint64_t permille = view.totalBytes == 0 ? 0 : done * 1000 / view.totalBytes;
    SendDlgItemMessageW(window, kBarId, PBM_SETPOS, static_cast<WPARAM>(permille > 990 ? 990 : permille), 0);
    if (!state.shown && GetTickCount64() - state.startTick >= 500) {
        ShowWindow(window, SW_SHOWNORMAL);
        state.shown = true;
    }
}

void CreateChildren(HWND window, WindowState& state)
{
    RECT client = {};
    GetClientRect(window, &client);
    const int margin = Scale(16, state.dpi);
    const int width = client.right - 2 * margin;
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    const DWORD textStyle = WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_ENDELLIPSIS;
    CreateWindowExW(0, L"STATIC", L"", textStyle, margin, Scale(14, state.dpi), width, Scale(20, state.dpi), window,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLineId)), instance, nullptr);
    CreateWindowExW(0, L"STATIC", L"", textStyle, margin, Scale(38, state.dpi), width, Scale(20, state.dpi), window,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCountId)), instance, nullptr);
    HWND bar = CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE, margin, Scale(64, state.dpi), width,
                               Scale(16, state.dpi), window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kBarId)),
                               instance, nullptr);
    SendMessageW(bar, PBM_SETRANGE32, 0, 1000);
    const int buttonWidth = Scale(96, state.dpi);
    CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                    client.right - margin - buttonWidth, Scale(94, state.dpi), buttonWidth, Scale(28, state.dpi), window,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDCANCEL)), instance, nullptr);
    for (HWND child = GetWindow(window, GW_CHILD); child != nullptr; child = GetWindow(child, GW_HWNDNEXT)) {
        SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(state.font), TRUE);
    }
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* state = reinterpret_cast<WindowState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    switch (message) {
    case WM_CREATE: {
        state = static_cast<WindowState*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        CreateChildren(window, *state);
        SetTimer(window, kTimerId, 100, nullptr);
        return 0;
    }
    case WM_TIMER:
        Refresh(window, *state);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) {
            RequestCancel(window, *state);
        }
        return 0;
    case WM_CLOSE:
        // Closing the window is the same as Cancel, and the window is destroyed after the jobs have stopped.
        RequestCancel(window, *state);
        return 0;
    case WM_CTLCOLORSTATIC:
        SetBkColor(reinterpret_cast<HDC>(wParam), GetSysColor(COLOR_WINDOW));
        return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
    case WM_DESTROY:
        KillTimer(window, kTimerId);
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

}  // namespace

void RunProgressWindow(Progress& progress)
{
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    WindowState state;
    state.progress = &progress;
    state.startTick = GetTickCount64();
    HDC screen = GetDC(nullptr);
    state.dpi = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(nullptr, screen);
    NONCLIENTMETRICSW metrics = {};
    metrics.cbSize = sizeof(metrics);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
    state.font = CreateFontIndirectW(&metrics.lfMessageFont);

    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    windowClass.lpszClassName = kClassName;
    windowClass.hIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_TGZ), IMAGE_ICON,
                                                      GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0));
    windowClass.hIconSm = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_TGZ), IMAGE_ICON,
                                                        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
    RegisterClassExW(&windowClass);

    const DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT frame = {0, 0, Scale(440, state.dpi), Scale(136, state.dpi)};
    AdjustWindowRectEx(&frame, style, FALSE, 0);
    const int width = frame.right - frame.left;
    const int height = frame.bottom - frame.top;
    POINT cursor = {};
    GetCursorPos(&cursor);
    MONITORINFO monitor = {};
    monitor.cbSize = sizeof(monitor);
    GetMonitorInfoW(MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST), &monitor);
    const int x = monitor.rcWork.left + (monitor.rcWork.right - monitor.rcWork.left - width) / 2;
    const int y = monitor.rcWork.top + (monitor.rcWork.bottom - monitor.rcWork.top - height) / 2;

    HWND window = CreateWindowExW(0, kClassName, L"AddToTarGz", style, x, y, width, height, nullptr, nullptr,
                                  instance, &state);
    if (window != nullptr) {
        MSG message = {};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            if (!IsDialogMessageW(window, &message)) {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
    } else {
        // Without a window, the jobs still run to the end, and the report has the outcome.
        while (!progress.finished.load()) {
            Sleep(100);
        }
    }
    DeleteObject(state.font);
}
