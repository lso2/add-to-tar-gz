// menu_entries.h: the commands the Explorer menu shows for a selection, in menu order, with verbs and labels.
// Globals: MenuCommand, MenuEntry, BuildMenuEntries.
// Doc: docs/notes/src/menu/menu_entries.md
#pragma once
#include <string>
#include <vector>
#include "selection.h"
#include "settings.h"

enum class MenuCommand { AddOne, Add, AddOther, ExtractHere, ExtractFolder, ExtractTo, Settings };

struct MenuEntry {
    MenuCommand command;
    const wchar_t* verb;  // language-independent name for InvokeCommand and GetCommandString
    std::wstring label;
    const wchar_t* help;
};

// Empty when the selection gets no Add or Extract entry. The one-archive entry is first, and Settings... is last,
// after the action entries.
std::vector<MenuEntry> BuildMenuEntries(const SelectionPlan& plan, const Settings& settings);
