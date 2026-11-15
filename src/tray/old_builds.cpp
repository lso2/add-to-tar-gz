// old_builds.cpp: removes the binaries of earlier builds from the AddToTarGz folder.
// Globals: none beyond old_builds.h.
// Doc: docs/notes/src/tray/old_builds.md
#include "old_builds.h"
#include "names.h"
#include "paths.h"
#include <windows.h>
#include <string>

namespace {

constexpr wchar_t kBuildPrefix[] = L"build-";

// The files Installer.iss puts into a build folder.
const wchar_t* const kBinaries[] = {L"AddToTarGz.exe", L"AddToTarGzTray.exe", L"targz_context.dll", L"7za.exe",
                                    L"7-Zip License.txt"};

void RemoveBinaries(const std::wstring& folder)
{
    for (const wchar_t* name : kBinaries) {
        DeleteFileW(ExtendedPath(JoinPath(folder, name)).c_str());
    }
}

}  // namespace

void RemoveOldBuilds()
{
    const std::wstring own = ModuleFolder(nullptr);
    // A copy outside a build folder, such as bin\x64\Release, has no sibling builds to remove.
    if (own.empty() || LeafName(own).rfind(kBuildPrefix, 0) != 0) {
        return;
    }
    const std::wstring root = ParentFolder(own);
    RemoveBinaries(root);
    WIN32_FIND_DATAW entry = {};
    HANDLE find = FindFirstFileExW(ExtendedPath(JoinPath(root, std::wstring(kBuildPrefix) + L"*")).c_str(),
                                   FindExInfoBasic, &entry, FindExSearchLimitToDirectories, nullptr, 0);
    if (find == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        if ((entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            continue;
        }
        const std::wstring folder = JoinPath(root, entry.cFileName);
        if (CompareStringOrdinal(folder.c_str(), -1, own.c_str(), -1, TRUE) == CSTR_EQUAL) {
            continue;
        }
        RemoveBinaries(folder);
        // Succeeds only for an empty folder.
        RemoveDirectoryW(ExtendedPath(folder).c_str());
    } while (FindNextFileW(find, &entry));
    FindClose(find);
}
