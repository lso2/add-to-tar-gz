// context_menu.h: the shell extension object Explorer creates for each right-click on files and folders.
// Explorer passes the selection through IShellExtInit, and IContextMenu adds the AddToTarGz submenu, or the
// entries themselves when the Submenu setting is off, and runs the chosen entry.
// Globals: ContextMenu.
// Doc: docs/notes/src/menu/context_menu.md
#pragma once
#include <windows.h>
#include <shlobj.h>
#include <vector>
#include "menu_entries.h"
#include "selection.h"
#include "settings.h"

class ContextMenu final : public IShellExtInit, public IContextMenu {
public:
    ContextMenu();
    ~ContextMenu();

    IFACEMETHODIMP QueryInterface(REFIID riid, void** object) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

    IFACEMETHODIMP Initialize(PCIDLIST_ABSOLUTE folder, IDataObject* data, HKEY progId) override;

    IFACEMETHODIMP QueryContextMenu(HMENU menu, UINT index, UINT firstId, UINT lastId, UINT flags) override;
    IFACEMETHODIMP InvokeCommand(CMINVOKECOMMANDINFO* info) override;
    IFACEMETHODIMP GetCommandString(UINT_PTR id, UINT type, UINT* reserved, CHAR* name, UINT size) override;

private:
    void Plan();
    const MenuEntry* FindByVerb(const wchar_t* verb) const;

    LONG refs_ = 1;
    std::vector<SelectedItem> items_;
    Settings settings_;
    std::vector<MenuEntry> entries_;
    bool planned_ = false;
    HBITMAP bitmap_ = nullptr;
};
