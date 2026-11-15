// names.cpp: naming rules for new archives and extraction folders.
// Globals: none beyond names.h.
// Doc: docs/notes/src/shared/names.md
#include "names.h"
#include "paths.h"
#include <windows.h>

const wchar_t kPartialSuffix[] = L".partial";

namespace {

bool EndsWithNoCase(const std::wstring& text, const wchar_t* tail)
{
    const size_t tailLength = wcslen(tail);
    if (text.size() < tailLength) {
        return false;
    }
    return CompareStringOrdinal(text.c_str() + (text.size() - tailLength), static_cast<int>(tailLength),
                                tail, static_cast<int>(tailLength), TRUE) == CSTR_EQUAL;
}

}  // namespace

const wchar_t* ExtSuffix(ArchiveExt ext)
{
    return ext == ArchiveExt::Tgz ? L".tgz" : L".tar.gz";
}

const wchar_t* ExtLabel(ArchiveExt ext)
{
    return ext == ArchiveExt::Tgz ? L"tgz" : L"tar.gz";
}

bool IsArchiveFileName(const std::wstring& name)
{
    return EndsWithNoCase(name, L".tar.gz") || EndsWithNoCase(name, L".tgz");
}

std::wstring ArchiveStem(const std::wstring& name)
{
    size_t cut = 0;
    if (EndsWithNoCase(name, L".tar.gz")) {
        cut = 7;
    } else if (EndsWithNoCase(name, L".tgz")) {
        cut = 4;
    }
    // A file named just ".tgz" is used whole, for a folder name that is never empty.
    if (cut == 0 || cut == name.size()) {
        return name;
    }
    return name.substr(0, name.size() - cut);
}

std::wstring ItemBaseName(const std::wstring& leaf, bool isFolder)
{
    if (isFolder) {
        return leaf;
    }
    const size_t dot = leaf.find_last_of(L'.');
    // ".htaccess" and "Makefile" are used whole.
    if (dot == std::wstring::npos || dot == 0) {
        return leaf;
    }
    return leaf.substr(0, dot);
}

std::wstring FolderBaseName(const std::wstring& folder)
{
    if (folder.size() >= 2 && folder.size() <= 3 && folder[1] == L':') {
        return folder.substr(0, 1);
    }
    return LeafName(folder);
}

std::wstring LeafName(const std::wstring& path)
{
    std::wstring trimmed = path;
    while (trimmed.size() > 1 && (trimmed.back() == L'\\' || trimmed.back() == L'/')) {
        trimmed.pop_back();
    }
    const size_t cut = trimmed.find_last_of(L"\\/");
    return cut == std::wstring::npos ? trimmed : trimmed.substr(cut + 1);
}

std::wstring ParentFolder(const std::wstring& path)
{
    std::wstring trimmed = path;
    while (trimmed.size() > 3 && (trimmed.back() == L'\\' || trimmed.back() == L'/')) {
        trimmed.pop_back();
    }
    const size_t cut = trimmed.find_last_of(L"\\/");
    if (cut == std::wstring::npos) {
        return std::wstring();
    }
    if (cut == 2 && trimmed.size() > 2 && trimmed[1] == L':') {
        return trimmed.substr(0, 3);
    }
    return trimmed.substr(0, cut);
}

bool IsNameTaken(const std::wstring& fullPath, const std::wstring& ownPartial)
{
    if (PathExists(fullPath)) {
        return true;
    }
    const std::wstring partial = fullPath + kPartialSuffix;
    if (!ownPartial.empty() &&
        CompareStringOrdinal(partial.c_str(), -1, ownPartial.c_str(), -1, TRUE) == CSTR_EQUAL) {
        return false;
    }
    return PathExists(partial);
}

std::wstring FirstFreeName(const std::wstring& folder, const std::wstring& base, const std::wstring& suffix,
                           const std::wstring& ownPartial)
{
    for (int number = 1; number <= 9999; ++number) {
        std::wstring leaf = base;
        if (number > 1) {
            leaf += L" (" + std::to_wstring(number) + L")";
        }
        const std::wstring candidate = JoinPath(folder, leaf + suffix);
        if (!IsNameTaken(candidate, ownPartial)) {
            return candidate;
        }
    }
    return std::wstring();
}
