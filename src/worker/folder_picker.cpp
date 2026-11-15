// folder_picker.cpp: IFileOpenDialog in folder mode for --pick-dest, centered on the monitor with the cursor.
// Globals: none beyond folder_picker.h.
// Doc: docs/notes/src/worker/folder_picker.md
#include "folder_picker.h"
#include <commctrl.h>
#include <new>
#include <shobjidl.h>

namespace {

// The top-left corner that centers a window of this size on the work area of the monitor with the cursor. The
// progress window in progress_window.cpp is placed with the same calculation.
POINT CenteredOnCursorMonitor(int width, int height)
{
    POINT cursor = {};
    GetCursorPos(&cursor);
    MONITORINFO monitor = {};
    monitor.cbSize = sizeof(monitor);
    GetMonitorInfoW(MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST), &monitor);
    return {monitor.rcWork.left + (monitor.rcWork.right - monitor.rcWork.left - width) / 2,
            monitor.rcWork.top + (monitor.rcWork.bottom - monitor.rcWork.top - height) / 2};
}

// The dialog restores its last position before it is shown, or the top-left corner of the screen on first use.
// The SetWindowPos call that shows it is changed to the centered position.
LRESULT CALLBACK CenterWhenShown(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
{
    if (message == WM_WINDOWPOSCHANGING) {
        auto* position = reinterpret_cast<WINDOWPOS*>(lParam);
        if ((position->flags & SWP_SHOWWINDOW) != 0) {
            RECT frame = {};
            GetWindowRect(window, &frame);
            const bool sizeUnchanged = (position->flags & SWP_NOSIZE) != 0;
            const POINT corner = CenteredOnCursorMonitor(sizeUnchanged ? frame.right - frame.left : position->cx,
                                                         sizeUnchanged ? frame.bottom - frame.top : position->cy);
            position->x = corner.x;
            position->y = corner.y;
            position->flags &= ~SWP_NOMOVE;
        }
    } else if (message == WM_NCDESTROY) {
        RemoveWindowSubclass(window, CenterWhenShown, id);
    }
    return DefSubclassProc(window, message, wParam, lParam);
}

// The dialog window exists from the first folder change on, and the subclass is installed there. An owner window
// would take the dialog's taskbar button.
class SubclassAtFirstFolder : public IFileDialogEvents {
public:
    IFACEMETHODIMP QueryInterface(REFIID riid, void** object) override
    {
        if (object == nullptr) {
            return E_POINTER;
        }
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IFileDialogEvents)) {
            *object = static_cast<IFileDialogEvents*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    IFACEMETHODIMP_(ULONG) AddRef() override
    {
        return static_cast<ULONG>(InterlockedIncrement(&refs_));
    }
    IFACEMETHODIMP_(ULONG) Release() override
    {
        const LONG count = InterlockedDecrement(&refs_);
        if (count == 0) {
            delete this;
        }
        return static_cast<ULONG>(count);
    }
    IFACEMETHODIMP OnFolderChange(IFileDialog* dialog) override
    {
        IOleWindow* oleWindow = nullptr;
        if (!subclassed_ && SUCCEEDED(dialog->QueryInterface(IID_PPV_ARGS(&oleWindow)))) {
            HWND window = nullptr;
            if (SUCCEEDED(oleWindow->GetWindow(&window)) && window != nullptr) {
                subclassed_ = SetWindowSubclass(window, CenterWhenShown, 1, 0) != FALSE;
            }
            oleWindow->Release();
        }
        return S_OK;
    }
    IFACEMETHODIMP OnFileOk(IFileDialog*) override { return S_OK; }
    IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) override { return S_OK; }
    IFACEMETHODIMP OnSelectionChange(IFileDialog*) override { return S_OK; }
    IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) override { return S_OK; }
    IFACEMETHODIMP OnTypeChange(IFileDialog*) override { return S_OK; }
    IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) override { return S_OK; }

private:
    LONG refs_ = 1;
    bool subclassed_ = false;
};

}  // namespace

HRESULT PickFolder(const std::wstring& startFolder, std::wstring& picked)
{
    IFileOpenDialog* dialog = nullptr;
    HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (FAILED(result)) {
        return result;
    }
    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    dialog->SetTitle(L"Extract to");
    dialog->SetOkButtonLabel(L"Extract");
    IShellItem* start = nullptr;
    if (!startFolder.empty() &&
        SUCCEEDED(SHCreateItemFromParsingName(startFolder.c_str(), nullptr, IID_PPV_ARGS(&start)))) {
        dialog->SetFolder(start);
        start->Release();
    }
    SubclassAtFirstFolder* events = new (std::nothrow) SubclassAtFirstFolder();
    DWORD cookie = 0;
    const bool advised = events != nullptr && SUCCEEDED(dialog->Advise(events, &cookie));
    result = dialog->Show(nullptr);
    if (advised) {
        dialog->Unadvise(cookie);
    }
    if (events != nullptr) {
        events->Release();
    }
    if (SUCCEEDED(result)) {
        IShellItem* item = nullptr;
        result = dialog->GetResult(&item);
        if (SUCCEEDED(result)) {
            PWSTR path = nullptr;
            result = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
            if (SUCCEEDED(result)) {
                picked = path;
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }
    dialog->Release();
    return result;
}
