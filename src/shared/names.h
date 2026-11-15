// names.h: naming rules for new archives and extraction folders.
// The menu label in targz_context.dll and the output of AddToTarGz.exe are both built from these rules.
// Globals: ArchiveExt, ExtSuffix, ExtLabel, kPartialSuffix, IsArchiveFileName, ArchiveStem,
//          ItemBaseName, FolderBaseName, LeafName, ParentFolder, IsNameTaken, FirstFreeName.
// Doc: docs/notes/src/shared/names.md
#pragma once
#include <string>

enum class ArchiveExt { TarGz, Tgz };

// ".tar.gz" or ".tgz".
const wchar_t* ExtSuffix(ArchiveExt ext);

// "tar.gz" or "tgz", for menu text.
const wchar_t* ExtLabel(ArchiveExt ext);

// Suffix on work in progress: "site.tar.gz.partial" while compressing, "site.partial" while extracting.
extern const wchar_t kPartialSuffix[];

// True for names ending in .tar.gz or .tgz, in any letter case.
bool IsArchiveFileName(const std::wstring& name);

// "site.tar.gz" gives "site" and "site.tgz" gives "site". Other names are returned unchanged.
std::wstring ArchiveStem(const std::wstring& name);

// Archive base name for an item: a folder's whole name, or a file's name without its last extension.
std::wstring ItemBaseName(const std::wstring& leaf, bool isFolder);

// Archive base name for several items in one archive: the name of their folder. A drive root gives its letter
// ("D"), the name 7-Zip gives that archive, and a share root gives its share name.
std::wstring FolderBaseName(const std::wstring& folder);

// Last path component, without trailing separators.
std::wstring LeafName(const std::wstring& path);

// Folder holding the path, without a trailing separator. A drive root has its separator ("C:\").
std::wstring ParentFolder(const std::wstring& path);

// A candidate counts as taken when it, or the same name plus kPartialSuffix, exists as a file or folder.
// `ownPartial` is the calling job's own .partial path, which does not count against a candidate.
bool IsNameTaken(const std::wstring& fullPath, const std::wstring& ownPartial = std::wstring());

// First name not taken among "<folder>\<base><suffix>", "<base> (2)<suffix>", "<base> (3)<suffix>"...
// Empty when 9999 candidates are all taken.
std::wstring FirstFreeName(const std::wstring& folder, const std::wstring& base, const std::wstring& suffix,
                           const std::wstring& ownPartial = std::wstring());
