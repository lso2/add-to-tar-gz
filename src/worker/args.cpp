// args.cpp: parses the AddToTarGz.exe command line.
// Globals: none beyond args.h.
// Doc: docs/notes/src/worker/args.md
#include "args.h"
#include "errors.h"
#include "paths.h"
#include <windows.h>
#include <shellapi.h>
#include <cwchar>

namespace {

std::wstring FullPath(const std::wstring& path)
{
    const DWORD needed = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
    if (needed == 0) {
        return path;
    }
    std::wstring buffer(needed, L'\0');
    const DWORD length = GetFullPathNameW(path.c_str(), needed, buffer.data(), nullptr);
    if (length == 0 || length >= needed) {
        return path;
    }
    buffer.resize(length);
    // A trailing separator would make the item's name empty, and only a drive root ends with one after this loop.
    while (buffer.size() > 3 && buffer.back() == L'\\') {
        buffer.pop_back();
    }
    return buffer;
}

// Paths from the menu DLL: NUL-separated UTF-16 ended by an empty string, in shared memory whose inherited
// handle is `value` in decimal.
bool ReadPathMemory(const std::wstring& value, std::vector<std::wstring>& paths, std::wstring& error)
{
    wchar_t* end = nullptr;
    const unsigned long long number = wcstoull(value.c_str(), &end, 10);
    if (value.empty() || end == nullptr || *end != L'\0' || number == 0) {
        error = L"--paths-memory takes a handle number, not \"" + value + L"\".";
        return false;
    }
    HANDLE memory = reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(number));
    const auto* view = static_cast<const wchar_t*>(MapViewOfFile(memory, FILE_MAP_READ, 0, 0, 0));
    if (view == nullptr) {
        error = L"Reading the paths from handle " + value + L" failed: " + WindowsErrorText(GetLastError());
        return false;
    }
    MEMORY_BASIC_INFORMATION region = {};
    const size_t chars = VirtualQuery(view, &region, sizeof(region)) != 0 ? region.RegionSize / sizeof(wchar_t) : 0;
    size_t at = 0;
    while (at < chars && view[at] != L'\0') {
        const size_t start = at;
        while (at < chars && view[at] != L'\0') {
            ++at;
        }
        paths.push_back(FullPath(std::wstring(view + start, at - start)));
        ++at;
    }
    UnmapViewOfFile(view);
    CloseHandle(memory);
    return true;
}

bool SetMode(Options& options, Mode mode, std::wstring& error)
{
    if (options.mode != Mode::None && options.mode != mode) {
        error = L"Use --add or --extract, not both.";
        return false;
    }
    options.mode = mode;
    return true;
}

// Option pairs that contradict each other, checked once every argument has been read.
bool CheckCombination(const Options& options, bool extGiven, bool levelGiven, std::wstring& error)
{
    const int placements = (options.here ? 1 : 0) + (options.dest.empty() ? 0 : 1) + (options.pickDest ? 1 : 0);
    if (options.mode == Mode::Extract && (extGiven || levelGiven)) {
        error = L"--ext and --level belong to --add.";
    } else if (options.mode == Mode::Extract && options.one) {
        error = L"--one belongs to --add.";
    } else if (options.mode == Mode::Add && placements > 0) {
        error = L"--here, --dest and --pick-dest belong to --extract.";
    } else if (placements > 1) {
        error = L"Use only one of --here, --dest and --pick-dest.";
    } else if (options.pickDest && options.silent) {
        error = L"--pick-dest opens a dialog, and --silent allows none.";
    } else if (!options.dest.empty() && !IsFolder(options.dest)) {
        error = options.dest + L" is not an existing folder.";
    } else {
        return true;
    }
    return false;
}

}  // namespace

bool ParseArgs(Options& options, std::wstring& error)
{
    int count = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &count);
    if (argv == nullptr) {
        error = L"The command line could not be split into arguments.";
        return false;
    }
    bool pathsOnly = false;
    bool extGiven = false;
    bool levelGiven = false;
    bool ok = true;
    for (int i = 1; i < count && ok; ++i) {
        const std::wstring arg = argv[i];
        const bool hasValue = i + 1 < count;
        if (pathsOnly || arg.rfind(L"--", 0) != 0) {
            options.paths.push_back(FullPath(arg));
        } else if (arg == L"--") {
            pathsOnly = true;
        } else if (arg == L"--add") {
            ok = SetMode(options, Mode::Add, error);
        } else if (arg == L"--extract") {
            ok = SetMode(options, Mode::Extract, error);
        } else if (arg == L"--one") {
            options.one = true;
        } else if (arg == L"--paths-memory" && hasValue) {
            ok = ReadPathMemory(argv[++i], options.paths, error);
        } else if (arg == L"--silent") {
            options.silent = true;
        } else if (arg == L"--here") {
            options.here = true;
        } else if (arg == L"--pick-dest") {
            options.pickDest = true;
        } else if (arg == L"--dest" && hasValue) {
            options.dest = FullPath(argv[++i]);
        } else if (arg == L"--ext" && hasValue) {
            const std::wstring value = argv[++i];
            extGiven = true;
            if (value == L"tar.gz") {
                options.ext = ArchiveExt::TarGz;
            } else if (value == L"tgz") {
                options.ext = ArchiveExt::Tgz;
            } else {
                error = L"Unknown extension \"" + value + L"\". Use tar.gz or tgz.";
                ok = false;
            }
        } else if (arg == L"--level" && hasValue) {
            const std::wstring value = argv[++i];
            levelGiven = true;
            if (value.size() == 1 && value[0] >= L'1' && value[0] <= L'9') {
                options.level = value[0] - L'0';
            } else {
                error = L"--level takes a number from 1 to 9, not \"" + value + L"\".";
                ok = false;
            }
        } else if (arg == L"--ext" || arg == L"--level" || arg == L"--dest" || arg == L"--paths-memory") {
            error = arg + L" needs a value after it.";
            ok = false;
        } else {
            error = L"Unknown option " + arg + L".";
            ok = false;
        }
    }
    LocalFree(argv);
    if (!ok) {
        return false;
    }
    if (options.mode == Mode::None) {
        error = L"Pick --add or --extract.";
        return false;
    }
    if (options.paths.empty()) {
        error = L"No file, folder or archive was given.";
        return false;
    }
    return CheckCombination(options, extGiven, levelGiven, error);
}

const wchar_t* UsageText()
{
    return L"AddToTarGz.exe --add [--one] [--ext tar.gz|tgz] [--level 1-9] [--silent] [--] <file or folder>...\r\n"
           L"AddToTarGz.exe --extract [--here | --dest <folder> | --pick-dest] [--silent] [--] "
           L"<archive.tar.gz or archive.tgz>...\r\n"
           L"\r\n"
           L"--add creates <name>.tar.gz beside each item.\r\n"
           L"--one puts every item into one archive named after the folder of the first item, in that folder.\r\n"
           L"--ext sets the extension --add uses. The default is tar.gz.\r\n"
           L"--level sets the gzip level --add uses, from 1 (fastest) to 9 (smallest). The default is 5.\r\n"
           L"--extract unpacks each archive into a new folder named after it, beside the archive.\r\n"
           L"--here puts the archive's entries straight into the archive's folder. A name already in use gets the "
           L"next free number.\r\n"
           L"--dest puts the new folders into another folder.\r\n"
           L"--pick-dest shows a folder dialog for that folder.\r\n"
           L"--silent shows no window or dialog. Results go to standard output and the exit code.\r\n"
           L"\r\n"
           L"Exit codes: 0 every job finished cleanly, 1 a job ended with warnings, failed or was canceled,\r\n"
           L"2 the command line was not valid.";
}
