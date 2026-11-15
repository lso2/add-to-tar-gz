// paths.cpp: path helpers shared by AddToTarGz.exe and targz_context.dll.
// Globals: none beyond paths.h.
// Doc: docs/notes/src/shared/paths.md
#include "paths.h"
#include <vector>

std::wstring ExtendedPath(const std::wstring& path)
{
    if (path.rfind(L"\\\\?\\", 0) == 0) {
        return path;
    }
    if (path.rfind(L"\\\\", 0) == 0) {
        return L"\\\\?\\UNC\\" + path.substr(2);
    }
    return L"\\\\?\\" + path;
}

std::wstring JoinPath(const std::wstring& folder, const std::wstring& name)
{
    if (folder.empty()) {
        return name;
    }
    const wchar_t last = folder.back();
    if (last == L'\\' || last == L'/') {
        return folder + name;
    }
    return folder + L"\\" + name;
}

bool PathExists(const std::wstring& path)
{
    return GetFileAttributesW(ExtendedPath(path).c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool IsFolder(const std::wstring& path)
{
    const DWORD attributes = GetFileAttributesW(ExtendedPath(path).c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool IsRootPath(const std::wstring& path)
{
    std::wstring trimmed = path;
    while (trimmed.size() > 2 && (trimmed.back() == L'\\' || trimmed.back() == L'/')) {
        trimmed.pop_back();
    }
    if (trimmed.size() == 2 && trimmed[1] == L':') {
        return true;
    }
    if (trimmed.rfind(L"\\\\", 0) == 0) {
        const size_t server = trimmed.find_first_of(L"\\/", 2);
        return server != std::wstring::npos && trimmed.find_first_of(L"\\/", server + 1) == std::wstring::npos;
    }
    return false;
}

std::wstring ModulePath(HMODULE module)
{
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        const DWORD length = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return std::wstring();
        }
        if (length < buffer.size()) {
            return std::wstring(buffer.data(), length);
        }
        buffer.resize(buffer.size() * 2);
    }
}

std::wstring ModuleFolder(HMODULE module)
{
    const std::wstring full = ModulePath(module);
    const size_t cut = full.find_last_of(L"\\/");
    return cut == std::wstring::npos ? std::wstring() : full.substr(0, cut);
}
