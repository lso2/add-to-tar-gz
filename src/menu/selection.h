// selection.h: reads the Explorer selection and decides whether it gets Add or Extract items, with their labels.
// Globals: SelectedItem, SelectionKind, SelectionPlan, ReadSelection, PlanSelection.
// Doc: docs/notes/src/menu/selection.md
#pragma once
#include <objidl.h>
#include <string>
#include <vector>
#include "names.h"
#include "settings.h"

struct SelectedItem {
    std::wstring path;
    bool exists = false;
    bool isFolder = false;
    bool isArchive = false;  // a file named *.tar.gz or *.tgz
    bool isRoot = false;     // a drive or share root
};

enum class SelectionKind { None, Add, Extract };

struct SelectionPlan {
    SelectionKind kind = SelectionKind::None;
    std::wstring addOneLabel;    // Add to "folder.tar.gz": every item in one archive, empty for one item
    std::wstring addLabel;       // Add to "site.tar.gz", or Add to 3 tar.gz archives
    std::wstring addOtherLabel;  // the same with the other extension
    std::wstring extractLabel;   // Extract to "site\", or Extract to 3 folders
};

// File system items of the selection, from its CF_HDROP data. Empty for virtual items.
std::vector<SelectedItem> ReadSelection(IDataObject* data);

// Extract when every item is an archive, Add otherwise. A drive or share root gets None. Several items to add
// get the one-archive label. The other labels are built only for items switched on in `settings`. Each label is
// a file system check per candidate name.
SelectionPlan PlanSelection(const std::vector<SelectedItem>& items, const Settings& settings);
