// selection.cpp: selection reading and menu label rules.
// Globals: none beyond selection.h.
// Doc: docs/notes/src/menu/selection.md
#include "selection.h"
#include "paths.h"
#include <shellapi.h>

namespace {

// Menus read "&" as a mnemonic marker, and a very long name would stretch the whole menu.
std::wstring MenuText(const std::wstring& name)
{
    std::wstring shortened = name;
    if (shortened.size() > 64) {
        shortened = shortened.substr(0, 30) + L"…" + shortened.substr(shortened.size() - 30);
    }
    std::wstring escaped;
    for (wchar_t c : shortened) {
        escaped.push_back(c);
        if (c == L'&') {
            escaped.push_back(L'&');
        }
    }
    return escaped;
}

std::wstring LeafOrFallback(const std::wstring& path, const std::wstring& fallback)
{
    return path.empty() ? fallback : LeafName(path);
}

SelectedItem Describe(const std::wstring& path)
{
    SelectedItem item;
    item.path = path;
    const DWORD attributes = GetFileAttributesW(ExtendedPath(path).c_str());
    item.exists = attributes != INVALID_FILE_ATTRIBUTES;
    item.isFolder = item.exists && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    item.isRoot = IsRootPath(path);
    item.isArchive = item.exists && !item.isFolder && IsArchiveFileName(LeafName(path));
    return item;
}

ArchiveExt OtherExt(ArchiveExt ext)
{
    return ext == ArchiveExt::Tgz ? ArchiveExt::TarGz : ArchiveExt::Tgz;
}

// One item shows the exact name FirstFreeName returns, the same call the worker makes. Several show a count.
std::wstring AddLabel(const std::vector<SelectedItem>& items, ArchiveExt ext)
{
    if (items.size() == 1) {
        const std::wstring base = ItemBaseName(LeafName(items[0].path), items[0].isFolder);
        const std::wstring suffix = ExtSuffix(ext);
        const std::wstring target = FirstFreeName(ParentFolder(items[0].path), base, suffix);
        return L"Add to \"" + MenuText(LeafOrFallback(target, base + suffix)) + L"\"";
    }
    return L"Add to " + std::to_wstring(items.size()) + L" " + ExtLabel(ext) + L" archives";
}

// Every item in one archive named after the first item's folder: the exact name the worker's --one takes.
std::wstring AddOneLabel(const std::vector<SelectedItem>& items, ArchiveExt ext)
{
    const std::wstring folder = ParentFolder(items[0].path);
    const std::wstring base = FolderBaseName(folder);
    const std::wstring suffix = ExtSuffix(ext);
    const std::wstring target = FirstFreeName(folder, base, suffix);
    return L"Add to \"" + MenuText(LeafOrFallback(target, base + suffix)) + L"\"";
}

std::wstring ExtractLabel(const std::vector<SelectedItem>& items)
{
    if (items.size() == 1) {
        const std::wstring stem = ArchiveStem(LeafName(items[0].path));
        const std::wstring target = FirstFreeName(ParentFolder(items[0].path), stem, L"");
        return L"Extract to \"" + MenuText(LeafOrFallback(target, stem)) + L"\\\"";
    }
    return L"Extract to " + std::to_wstring(items.size()) + L" folders";
}

}  // namespace

std::vector<SelectedItem> ReadSelection(IDataObject* data)
{
    std::vector<SelectedItem> items;
    FORMATETC format = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium = {};
    if (FAILED(data->GetData(&format, &medium))) {
        return items;
    }
    HDROP drop = static_cast<HDROP>(GlobalLock(medium.hGlobal));
    if (drop != nullptr) {
        const UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
        for (UINT i = 0; i < count; ++i) {
            const UINT length = DragQueryFileW(drop, i, nullptr, 0);
            if (length == 0) {
                continue;
            }
            std::wstring path(length + 1, L'\0');
            DragQueryFileW(drop, i, path.data(), length + 1);
            path.resize(length);
            items.push_back(Describe(path));
        }
        GlobalUnlock(medium.hGlobal);
    }
    ReleaseStgMedium(&medium);
    return items;
}

SelectionPlan PlanSelection(const std::vector<SelectedItem>& items, const Settings& settings)
{
    SelectionPlan plan;
    if (items.empty()) {
        return plan;
    }
    bool allArchives = true;
    bool anyRoot = false;
    for (const SelectedItem& item : items) {
        if (!item.exists) {
            return plan;
        }
        allArchives = allArchives && item.isArchive;
        anyRoot = anyRoot || item.isRoot;
    }
    if (allArchives) {
        plan.kind = SelectionKind::Extract;
        if (settings.itemExtractFolder) {
            plan.extractLabel = ExtractLabel(items);
        }
        return plan;
    }
    if (anyRoot) {
        return plan;
    }
    plan.kind = SelectionKind::Add;
    if (items.size() > 1) {
        plan.addOneLabel = AddOneLabel(items, settings.ext);
    }
    if (settings.itemAdd) {
        plan.addLabel = AddLabel(items, settings.ext);
    }
    if (settings.itemAddOther) {
        plan.addOtherLabel = AddLabel(items, OtherExt(settings.ext));
    }
    return plan;
}
