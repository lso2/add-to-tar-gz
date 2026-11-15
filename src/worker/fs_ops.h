// fs_ops.h: file system steps of the jobs: sizes for progress, folder listing, the 7-Zip list file, and the final
// rename.
// Globals: ChildEntry, FolderSize, FileSize, ListChildren, WriteListFile, MoveToFreeName.
// Doc: docs/notes/src/worker/fs_ops.md
#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

struct ChildEntry {
    std::wstring name;
    bool isFolder = false;
    bool isReparsePoint = false;
};

// Bytes in every file below the folder. Reparse points count but are not entered.
uint64_t FolderSize(const std::wstring& folder, const std::atomic<bool>& cancel);

// Size of one file, or 0 when it cannot be read.
uint64_t FileSize(const std::wstring& file);

// Up to `limit` entries of the folder, without "." and "..".
std::vector<ChildEntry> ListChildren(const std::wstring& folder, size_t limit);

// A 7-Zip list file of `paths` in the temp folder: UTF-8, one quoted path per line. 7-Zip trims each line, and the
// quotes keep a name's leading and trailing spaces. Returns false with `error` set. The caller deletes the file.
bool WriteListFile(const std::vector<std::wstring>& paths, std::wstring& listPath, std::wstring& error);

// Renames `from` to `reserved`, or to FirstFreeName(folder, base, suffix, ownPartial) when another program took
// `reserved` meanwhile. Retries for 10 seconds while a scanner holds a file open.
// Returns the final path, or empty with `error` set.
std::wstring MoveToFreeName(const std::wstring& from, const std::wstring& reserved, const std::wstring& folder,
                            const std::wstring& base, const std::wstring& suffix, std::wstring& error,
                            const std::wstring& ownPartial = std::wstring());
