// context_menu.cpp: IShellExtInit and IContextMenu for the AddToTarGz submenu and its entries.
// Globals: none beyond context_menu.h.
// Doc: docs/notes/src/menu/context_menu.md
#include "context_menu.h"
#include "launch.h"
#include "menu_bitmap.h"
#include "module.h"
#include <new>
#include <shellapi.h>
#include <strsafe.h>

namespace {

constexpr wchar_t kSubmenuLabel[] = L"AddToTarGz";

std::wstring Widen(const char* text)
{
    const int length = MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0);
    if (length <= 0) {
        return std::wstring();
    }
    std::wstring wide(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_ACP, 0, text, -1, wide.data(), length);
    wide.resize(static_cast<size_t>(length - 1));
    return wide;
}

bool InsertEntry(HMENU menu, UINT position, UINT id, const std::wstring& label, HBITMAP bitmap)
{
    MENUITEMINFOW item = {};
    item.cbSize = sizeof(item);
    item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID | MIIM_STATE;
    item.fType = MFT_STRING;
    item.fState = MFS_ENABLED;
    item.wID = id;
    item.dwTypeData = const_cast<wchar_t*>(label.c_str());
    if (bitmap != nullptr) {
        item.fMask |= MIIM_BITMAP;
        item.hbmpItem = bitmap;
    }
    return InsertMenuItemW(menu, position, TRUE, &item) != FALSE;
}

bool InsertSeparator(HMENU menu, UINT position)
{
    MENUITEMINFOW item = {};
    item.cbSize = sizeof(item);
    item.fMask = MIIM_FTYPE;
    item.fType = MFT_SEPARATOR;
    return InsertMenuItemW(menu, position, TRUE, &item) != FALSE;
}

}  // namespace

ContextMenu::ContextMenu()
{
    ModuleAddRef();
}

ContextMenu::~ContextMenu()
{
    if (bitmap_ != nullptr) {
        DeleteObject(bitmap_);
    }
    ModuleRelease();
}

IFACEMETHODIMP ContextMenu::QueryInterface(REFIID riid, void** object)
{
    if (object == nullptr) {
        return E_POINTER;
    }
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IContextMenu)) {
        *object = static_cast<IContextMenu*>(this);
    } else if (IsEqualIID(riid, IID_IShellExtInit)) {
        *object = static_cast<IShellExtInit*>(this);
    } else {
        *object = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

IFACEMETHODIMP_(ULONG) ContextMenu::AddRef()
{
    return static_cast<ULONG>(InterlockedIncrement(&refs_));
}

IFACEMETHODIMP_(ULONG) ContextMenu::Release()
{
    const LONG count = InterlockedDecrement(&refs_);
    if (count == 0) {
        delete this;
    }
    return static_cast<ULONG>(count);
}

void ContextMenu::Plan()
{
    settings_ = ReadSettings();
    entries_ = BuildMenuEntries(PlanSelection(items_, settings_), settings_);
    planned_ = true;
}

const MenuEntry* ContextMenu::FindByVerb(const wchar_t* verb) const
{
    for (const MenuEntry& entry : entries_) {
        if (CompareStringOrdinal(verb, -1, entry.verb, -1, TRUE) == CSTR_EQUAL) {
            return &entry;
        }
    }
    return nullptr;
}

IFACEMETHODIMP ContextMenu::Initialize(PCIDLIST_ABSOLUTE, IDataObject* data, HKEY)
{
    if (data == nullptr) {
        return E_INVALIDARG;
    }
    try {
        items_ = ReadSelection(data);
        planned_ = false;
    } catch (const std::bad_alloc&) {
        return E_OUTOFMEMORY;
    }
    return items_.empty() ? E_FAIL : S_OK;
}

IFACEMETHODIMP ContextMenu::QueryContextMenu(HMENU menu, UINT index, UINT firstId, UINT lastId, UINT flags)
{
    if ((flags & CMF_DEFAULTONLY) != 0) {
        return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
    }
    try {
        Plan();
    } catch (const std::bad_alloc&) {
        return E_OUTOFMEMORY;
    }
    const UINT count = static_cast<UINT>(entries_.size());
    // The submenu entry takes the id after the commands.
    const UINT needed = settings_.submenu ? count + 1 : count;
    if (count == 0 || lastId < firstId || lastId - firstId + 1 < needed) {
        return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
    }
    if (settings_.menuIcons && bitmap_ == nullptr) {
        bitmap_ = CreateMenuBitmap(Module());
    }
    HBITMAP icon = settings_.menuIcons ? bitmap_ : nullptr;
    if (!settings_.submenu) {
        for (UINT i = 0; i < count; ++i) {
            if (!InsertEntry(menu, index + i, firstId + i, entries_[i].label, icon)) {
                return HRESULT_FROM_WIN32(GetLastError());
            }
        }
        return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, needed);
    }
    HMENU submenu = CreatePopupMenu();
    if (submenu == nullptr) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    UINT position = 0;
    for (UINT i = 0; i < count; ++i) {
        // Dividers set apart the one-archive entry from the entries after it, and Settings... from the archive
        // commands.
        const bool separated = i > 0 && (entries_[i].command == MenuCommand::Settings ||
                                         entries_[i - 1].command == MenuCommand::AddOne);
        if ((separated && !InsertSeparator(submenu, position++)) ||
            !InsertEntry(submenu, position++, firstId + i, entries_[i].label, nullptr)) {
            const DWORD code = GetLastError();
            DestroyMenu(submenu);
            return HRESULT_FROM_WIN32(code);
        }
    }
    MENUITEMINFOW top = {};
    top.cbSize = sizeof(top);
    top.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID | MIIM_STATE | MIIM_SUBMENU;
    top.fType = MFT_STRING;
    top.fState = MFS_ENABLED;
    top.wID = firstId + count;
    top.hSubMenu = submenu;
    top.dwTypeData = const_cast<wchar_t*>(kSubmenuLabel);
    if (icon != nullptr) {
        top.fMask |= MIIM_BITMAP;
        top.hbmpItem = icon;
    }
    if (!InsertMenuItemW(menu, index, TRUE, &top)) {
        const DWORD code = GetLastError();
        DestroyMenu(submenu);
        return HRESULT_FROM_WIN32(code);
    }
    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, needed);
}

IFACEMETHODIMP ContextMenu::InvokeCommand(CMINVOKECOMMANDINFO* info)
{
    if (info == nullptr) {
        return E_INVALIDARG;
    }
    try {
        if (!planned_) {
            Plan();
        }
        const bool unicode = info->cbSize >= sizeof(CMINVOKECOMMANDINFOEX) && (info->fMask & CMIC_MASK_UNICODE) != 0;
        const auto* extended = reinterpret_cast<const CMINVOKECOMMANDINFOEX*>(info);
        const MenuEntry* entry = nullptr;
        if (unicode && !IS_INTRESOURCE(extended->lpVerbW)) {
            entry = FindByVerb(extended->lpVerbW);
        } else if (!IS_INTRESOURCE(info->lpVerb)) {
            entry = FindByVerb(Widen(info->lpVerb).c_str());
        } else {
            const UINT_PTR offset = reinterpret_cast<UINT_PTR>(info->lpVerb);
            entry = offset < entries_.size() ? &entries_[offset] : nullptr;
        }
        // A command runs only when the current selection offers it.
        if (entry == nullptr) {
            return E_FAIL;
        }
        return LaunchCommand(entry->command, settings_, items_, info->hwnd);
    } catch (const std::bad_alloc&) {
        return E_OUTOFMEMORY;
    }
}

IFACEMETHODIMP ContextMenu::GetCommandString(UINT_PTR id, UINT type, UINT*, CHAR* name, UINT size)
{
    if (name == nullptr) {
        return E_INVALIDARG;
    }
    try {
        if (!planned_) {
            Plan();
        }
    } catch (const std::bad_alloc&) {
        return E_OUTOFMEMORY;
    }
    if (id >= entries_.size()) {
        return E_INVALIDARG;
    }
    const MenuEntry& entry = entries_[id];
    char narrow[256] = {};
    switch (type) {
    case GCS_VERBW:
        return StringCchCopyW(reinterpret_cast<wchar_t*>(name), size, entry.verb);
    case GCS_HELPTEXTW:
        return StringCchCopyW(reinterpret_cast<wchar_t*>(name), size, entry.help);
    case GCS_VERBA:
        WideCharToMultiByte(CP_ACP, 0, entry.verb, -1, narrow, sizeof(narrow), nullptr, nullptr);
        return StringCchCopyA(name, size, narrow);
    case GCS_HELPTEXTA:
        WideCharToMultiByte(CP_ACP, 0, entry.help, -1, narrow, sizeof(narrow), nullptr, nullptr);
        return StringCchCopyA(name, size, narrow);
    case GCS_VALIDATEW:
    case GCS_VALIDATEA:
        return S_OK;
    default:
        return E_NOTIMPL;
    }
}
