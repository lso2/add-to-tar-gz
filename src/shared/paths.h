// paths.h: path helpers shared by every AddToTarGz binary.
// Covers long paths (\\?\ prefix), joining, existence checks and the folder of a loaded module.
// Globals: ExtendedPath, JoinPath, PathExists, IsFolder, IsRootPath, ModulePath, ModuleFolder.
// Doc: docs/notes/src/shared/paths.md
#pragma once
#include <windows.h>
#include <string>

// Absolute path with the \\?\ (or \\?\UNC\) prefix. Win32 file calls accept it past MAX_PATH.
std::wstring ExtendedPath(const std::wstring& path);

// "C:\a" + "b" gives "C:\a\b", and "C:\" + "b" gives "C:\b".
std::wstring JoinPath(const std::wstring& folder, const std::wstring& name);

// True when a file or folder exists at the path.
bool PathExists(const std::wstring& path);

// True when the path is an existing folder.
bool IsFolder(const std::wstring& path);

// True for a drive root ("C:\") or a share root ("\\server\share").
bool IsRootPath(const std::wstring& path);

// Full path of the module file (the exe or the DLL), or empty when Windows reports an error.
std::wstring ModulePath(HMODULE module);

// Folder holding the module file, without a trailing separator.
std::wstring ModuleFolder(HMODULE module);
