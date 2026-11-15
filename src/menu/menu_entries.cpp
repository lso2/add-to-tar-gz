// menu_entries.cpp: menu entries from the selection plan and the item switches in the settings.
// Globals: none beyond menu_entries.h.
// Doc: docs/notes/src/menu/menu_entries.md
#include "menu_entries.h"

std::vector<MenuEntry> BuildMenuEntries(const SelectionPlan& plan, const Settings& settings)
{
    std::vector<MenuEntry> entries;
    if (plan.kind == SelectionKind::Add) {
        if (!plan.addOneLabel.empty()) {
            entries.push_back({MenuCommand::AddOne, L"addtotargz.addone", plan.addOneLabel,
                               L"Creates one archive holding every selected item, named after their folder."});
        }
        if (settings.itemAdd) {
            entries.push_back({MenuCommand::Add, L"addtotargz.add", plan.addLabel,
                               L"Creates one archive beside each selected item."});
        }
        if (settings.itemAddOther) {
            entries.push_back({MenuCommand::AddOther, L"addtotargz.addother", plan.addOtherLabel,
                               L"Creates one archive with the other extension beside each selected item."});
        }
    } else if (plan.kind == SelectionKind::Extract) {
        if (settings.itemExtractHere) {
            entries.push_back({MenuCommand::ExtractHere, L"addtotargz.extracthere", L"Extract here",
                               L"Extracts into this folder. A name already in use gets the next free number."});
        }
        if (settings.itemExtractFolder) {
            entries.push_back({MenuCommand::ExtractFolder, L"addtotargz.extract", plan.extractLabel,
                               L"Extracts each archive into a new folder named after it."});
        }
        if (settings.itemExtractTo) {
            entries.push_back({MenuCommand::ExtractTo, L"addtotargz.extractto", L"Extract to...",
                               L"Extracts each archive into a new folder inside a folder picked in a dialog."});
        }
    }
    if (!entries.empty() && settings.itemSettings) {
        entries.push_back({MenuCommand::Settings, L"addtotargz.settings",
                           settings.submenu ? L"Settings..." : L"AddToTarGz settings...",
                           L"Opens the AddToTarGz settings window."});
    }
    return entries;
}
