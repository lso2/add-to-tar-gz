// settings_dialog.cpp: dialog procedure of the Settings window: loads the settings into the controls and saves
// them with OK or Apply. The footer controls are passed to settings_footer.cpp.
// Globals: none beyond settings_dialog.h.
// Doc: docs/notes/src/tray/settings_dialog.md
#include "settings_dialog.h"
#include "autostart.h"
#include "names.h"
#include "resource_ids.h"
#include "settings.h"
#include "settings_footer.h"
#include "tray_ids.h"
#include "tray_window.h"
#include "update_result.h"
#include <string>

namespace {

struct CheckBox {
    int id;
    bool Settings::*member;
};

constexpr CheckBox kCheckBoxes[] = {
    {IDC_SUBMENU, &Settings::submenu},
    {IDC_ICONS, &Settings::menuIcons},
    {IDC_ITEM_ADD, &Settings::itemAdd},
    {IDC_ITEM_ADD_OTHER, &Settings::itemAddOther},
    {IDC_ITEM_EXTRACT_HERE, &Settings::itemExtractHere},
    {IDC_ITEM_EXTRACT_FOLDER, &Settings::itemExtractFolder},
    {IDC_ITEM_EXTRACT_TO, &Settings::itemExtractTo},
    {IDC_ITEM_SETTINGS, &Settings::itemSettings},
    {IDC_TRAY, &Settings::trayIcon},
};

HWND TrayOf(HWND dialog)
{
    return reinterpret_cast<HWND>(GetWindowLongPtrW(dialog, DWLP_USER));
}

ArchiveExt CheckedExt(HWND dialog)
{
    return IsDlgButtonChecked(dialog, IDC_EXT_TGZ) == BST_CHECKED ? ArchiveExt::Tgz : ArchiveExt::TarGz;
}

// The two Add labels follow the extension picked above them.
void UpdateAddLabels(HWND dialog)
{
    const ArchiveExt ext = CheckedExt(dialog);
    const ArchiveExt other = ext == ArchiveExt::Tgz ? ArchiveExt::TarGz : ArchiveExt::Tgz;
    SetDlgItemTextW(dialog, IDC_ITEM_ADD, (std::wstring(L"Add to \"name") + ExtSuffix(ext) + L"\"").c_str());
    SetDlgItemTextW(dialog, IDC_ITEM_ADD_OTHER, (std::wstring(L"Add to \"name") + ExtSuffix(other) + L"\"").c_str());
}

void Load(HWND dialog, const Settings& settings)
{
    CheckRadioButton(dialog, IDC_EXT_TARGZ, IDC_EXT_TGZ,
                     settings.ext == ArchiveExt::Tgz ? IDC_EXT_TGZ : IDC_EXT_TARGZ);
    CheckRadioButton(dialog, IDC_LEVEL_1, IDC_LEVEL_9,
                     settings.level == 1 ? IDC_LEVEL_1 : (settings.level == 9 ? IDC_LEVEL_9 : IDC_LEVEL_5));
    for (const CheckBox& box : kCheckBoxes) {
        CheckDlgButton(dialog, box.id, (settings.*box.member) ? BST_CHECKED : BST_UNCHECKED);
    }
    UpdateAddLabels(dialog);
}

bool Save(HWND dialog)
{
    Settings settings;
    settings.ext = CheckedExt(dialog);
    settings.level = IsDlgButtonChecked(dialog, IDC_LEVEL_1) == BST_CHECKED
                         ? 1
                         : (IsDlgButtonChecked(dialog, IDC_LEVEL_9) == BST_CHECKED ? 9 : 5);
    for (const CheckBox& box : kCheckBoxes) {
        settings.*box.member = IsDlgButtonChecked(dialog, box.id) == BST_CHECKED;
    }
    std::wstring error;
    if (!WriteSettings(settings, error) || !SetStartAtSignIn(settings.trayIcon, error)) {
        MessageBoxW(dialog, error.c_str(), L"AddToTarGz Settings", MB_OK | MB_ICONERROR);
        return false;
    }
    PostMessageW(TrayOf(dialog), WM_ATG_SETTINGS_APPLIED, 0, 0);
    return true;
}

void SetIcons(HWND dialog)
{
    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(dialog, GWLP_HINSTANCE));
    const int sizes[][2] = {{GetSystemMetrics(SM_CXSMICON), ICON_SMALL}, {GetSystemMetrics(SM_CXICON), ICON_BIG}};
    for (const auto& size : sizes) {
        HICON icon = static_cast<HICON>(
            LoadImageW(instance, MAKEINTRESOURCEW(IDI_TGZ), IMAGE_ICON, size[0], size[0], LR_SHARED));
        SendMessageW(dialog, WM_SETICON, static_cast<WPARAM>(size[1]), reinterpret_cast<LPARAM>(icon));
    }
}

INT_PTR CALLBACK SettingsProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        SetWindowLongPtrW(dialog, DWLP_USER, lParam);
        SetIcons(dialog);
        SetDlgItemTextW(dialog, IDC_ITEM_EXTRACT_HERE, L"Extract here");
        SetDlgItemTextW(dialog, IDC_ITEM_EXTRACT_FOLDER, L"Extract to \"name\\\"");
        SetDlgItemTextW(dialog, IDC_ITEM_EXTRACT_TO, L"Extract to...");
        SetDlgItemTextW(dialog, IDC_ITEM_SETTINGS, L"Settings...");
        InitFooter(dialog);
        Load(dialog, ReadSettings());
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_EXT_TARGZ:
        case IDC_EXT_TGZ:
            if (HIWORD(wParam) == BN_CLICKED) {
                UpdateAddLabels(dialog);
            }
            return TRUE;
        case IDC_APPLY:
            Save(dialog);
            return TRUE;
        case IDOK:
            if (Save(dialog)) {
                DestroyWindow(dialog);
            }
            return TRUE;
        case IDCANCEL:
            DestroyWindow(dialog);
            return TRUE;
        default:
            return HIWORD(wParam) == BN_CLICKED && OnFooterCommand(dialog, LOWORD(wParam)) ? TRUE : FALSE;
        }
    case WM_DRAWITEM:
        return OnFooterDrawItem(*reinterpret_cast<const DRAWITEMSTRUCT*>(lParam)) ? TRUE : FALSE;
    case WM_ATG_RELOAD:
        Load(dialog, ReadSettings());
        return TRUE;
    case WM_ATG_UPDATE_RESULT:
        ShowUpdateResult(dialog, *reinterpret_cast<const UpdateResult*>(lParam));
        return TRUE;
    case WM_DPICHANGED:
        UpdateFooterDpi(dialog, HIWORD(wParam));
        // With FALSE, the dialog manager moves and scales the window to the suggested rectangle.
        return FALSE;
    case WM_DESTROY:
        CloseFooter(dialog);
        PostMessageW(TrayOf(dialog), WM_ATG_SETTINGS_CLOSED, 0, 0);
        return FALSE;
    default:
        return FALSE;
    }
}

}  // namespace

HWND OpenSettingsDialog(HINSTANCE instance, HWND trayWindow)
{
    HWND dialog = CreateDialogParamW(instance, MAKEINTRESOURCEW(IDD_SETTINGS), nullptr, SettingsProc,
                                     reinterpret_cast<LPARAM>(trayWindow));
    if (dialog != nullptr) {
        ShowWindow(dialog, SW_SHOWNORMAL);
        SetForegroundWindow(dialog);
    }
    return dialog;
}
