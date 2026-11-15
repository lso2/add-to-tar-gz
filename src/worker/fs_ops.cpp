// fs_ops.cpp: file system steps of the jobs.
// Globals: none beyond fs_ops.h.
// Doc: docs/notes/src/worker/fs_ops.md
#include "fs_ops.h"
#include "errors.h"
#include "names.h"
#include "paths.h"

namespace {

bool IsDots(const wchar_t* name)
{
    return wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0;
}

}  // namespace

uint64_t FolderSize(const std::wstring& folder, const std::atomic<bool>& cancel)
{
    uint64_t total = 0;
    std::vector<std::wstring> pending{folder};
    while (!pending.empty() && !cancel.load()) {
        const std::wstring current = pending.back();
        pending.pop_back();
        WIN32_FIND_DATAW data = {};
        HANDLE find = FindFirstFileExW(ExtendedPath(JoinPath(current, L"*")).c_str(), FindExInfoBasic, &data,
                                       FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
        if (find == INVALID_HANDLE_VALUE) {
            continue;
        }
        do {
            if (IsDots(data.cFileName)) {
                continue;
            }
            const bool folderEntry = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            const bool reparse = (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
            if (folderEntry && !reparse) {
                pending.push_back(JoinPath(current, data.cFileName));
            } else if (!folderEntry) {
                total += (static_cast<uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
            }
        } while (FindNextFileW(find, &data));
        FindClose(find);
    }
    return total;
}

uint64_t FileSize(const std::wstring& file)
{
    WIN32_FILE_ATTRIBUTE_DATA data = {};
    if (!GetFileAttributesExW(ExtendedPath(file).c_str(), GetFileExInfoStandard, &data)) {
        return 0;
    }
    return (static_cast<uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
}

std::vector<ChildEntry> ListChildren(const std::wstring& folder, size_t limit)
{
    std::vector<ChildEntry> entries;
    WIN32_FIND_DATAW data = {};
    HANDLE find = FindFirstFileExW(ExtendedPath(JoinPath(folder, L"*")).c_str(), FindExInfoBasic, &data,
                                   FindExSearchNameMatch, nullptr, 0);
    if (find == INVALID_HANDLE_VALUE) {
        return entries;
    }
    do {
        if (IsDots(data.cFileName)) {
            continue;
        }
        ChildEntry entry;
        entry.name = data.cFileName;
        entry.isFolder = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        entry.isReparsePoint = (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
        entries.push_back(entry);
    } while (entries.size() < limit && FindNextFileW(find, &data));
    FindClose(find);
    return entries;
}

std::wstring MoveToFreeName(const std::wstring& from, const std::wstring& reserved, const std::wstring& folder,
                            const std::wstring& base, const std::wstring& suffix, std::wstring& error,
                            const std::wstring& ownPartial)
{
    std::wstring target = reserved;
    DWORD lastError = 0;
    for (int attempt = 0; attempt < 40; ++attempt) {
        if (MoveFileExW(ExtendedPath(from).c_str(), ExtendedPath(target).c_str(), 0)) {
            return target;
        }
        lastError = GetLastError();
        if (lastError == ERROR_ALREADY_EXISTS || lastError == ERROR_FILE_EXISTS) {
            target = FirstFreeName(folder, base, suffix, ownPartial);
            if (target.empty()) {
                break;
            }
        } else if (lastError == ERROR_ACCESS_DENIED || lastError == ERROR_SHARING_VIOLATION) {
            // A virus scanner opening the new files blocks the rename for a moment.
            Sleep(250);
        } else {
            break;
        }
    }
    error = L"Renaming " + from + L" to " + (target.empty() ? reserved : target) + L" failed: " +
            WindowsErrorText(lastError);
    return std::wstring();
}

bool WriteListFile(const std::vector<std::wstring>& paths, std::wstring& listPath, std::wstring& error)
{
    wchar_t folder[MAX_PATH + 1] = {};
    const DWORD length = GetTempPathW(MAX_PATH + 1, folder);
    if (length == 0 || length > MAX_PATH) {
        error = L"GetTempPathW failed: " + WindowsErrorText(GetLastError());
        return false;
    }
    wchar_t name[MAX_PATH] = {};
    if (GetTempFileNameW(folder, L"atg", 0, name) == 0) {
        error = L"Creating a list file in " + std::wstring(folder) + L" failed: " + WindowsErrorText(GetLastError());
        return false;
    }
    listPath = name;
    std::wstring text;
    for (const std::wstring& path : paths) {
        text += L"\"" + path + L"\"\n";
    }
    const int bytes =
        WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data(), bytes, nullptr, nullptr);
    HANDLE file = CreateFileW(name, GENERIC_WRITE, 0, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    DWORD code = ERROR_SUCCESS;
    if (file == INVALID_HANDLE_VALUE) {
        code = GetLastError();
    } else {
        DWORD written = 0;
        if (!WriteFile(file, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr)) {
            code = GetLastError();
        } else if (written != utf8.size()) {
            code = ERROR_WRITE_FAULT;
        }
        CloseHandle(file);
    }
    if (code != ERROR_SUCCESS) {
        error = L"Writing the list file " + listPath + L" failed: " + WindowsErrorText(code);
        DeleteFileW(name);
        return false;
    }
    return true;
}
