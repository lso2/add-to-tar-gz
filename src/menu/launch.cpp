// launch.cpp: builds the command line of the worker or the tray app and starts it.
// Globals: none beyond launch.h.
// Doc: docs/notes/src/menu/launch.md
#include "launch.h"
#include "cmdline.h"
#include "errors.h"
#include "module.h"
#include "names.h"
#include "paths.h"
#include <cstring>

namespace {

constexpr size_t kCommandLineBudget = 32000;

HRESULT ShowStartError(const std::wstring& exe, const std::wstring& step, DWORD code, HWND owner)
{
    const std::wstring text = LeafName(exe) + L" did not start.\r\n" + exe + L"\r\n" + step + WindowsErrorText(code);
    MessageBoxW(owner, text.c_str(), L"AddToTarGz", MB_OK | MB_ICONERROR);
    return HRESULT_FROM_WIN32(code);
}

// Starts exe. `inherited`, when not null, is the one handle the new process inherits.
HRESULT StartProcess(const std::wstring& exe, std::wstring commandLine, HANDLE inherited, HWND owner)
{
    STARTUPINFOEXW startup = {};
    startup.StartupInfo.cb = sizeof(STARTUPINFOW);
    std::vector<unsigned char> attributes;
    DWORD flags = 0;
    if (inherited != nullptr) {
        SIZE_T size = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
        attributes.resize(size);
        auto* list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
        if (!InitializeProcThreadAttributeList(list, 1, 0, &size)) {
            return ShowStartError(exe, L"InitializeProcThreadAttributeList failed: ", GetLastError(), owner);
        }
        // Explorer has many inheritable handles, and the worker gets only this one.
        if (!UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &inherited, sizeof(inherited),
                                       nullptr, nullptr)) {
            const DWORD code = GetLastError();
            DeleteProcThreadAttributeList(list);
            return ShowStartError(exe, L"UpdateProcThreadAttribute failed: ", code, owner);
        }
        startup.StartupInfo.cb = sizeof(startup);
        startup.lpAttributeList = list;
        flags = EXTENDED_STARTUPINFO_PRESENT;
    }
    PROCESS_INFORMATION info = {};
    const BOOL started = CreateProcessW(exe.c_str(), commandLine.data(), nullptr, nullptr, inherited != nullptr, flags,
                                        nullptr, nullptr, &startup.StartupInfo, &info);
    const DWORD code = started ? ERROR_SUCCESS : GetLastError();
    if (startup.lpAttributeList != nullptr) {
        DeleteProcThreadAttributeList(startup.lpAttributeList);
    }
    if (!started) {
        return ShowStartError(exe, std::wstring(), code, owner);
    }
    // Gives the new process Explorer's foreground right, for its dialogs and the settings window.
    AllowSetForegroundWindow(info.dwProcessId);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);
    return S_OK;
}

// Every path of the one-archive command in one piece: NUL-separated UTF-16 ended by an empty string, in
// pagefile-backed shared memory the worker inherits. Command line batches would make one archive per batch.
HRESULT StartWithPathMemory(const std::wstring& exe, std::wstring commandLine, const std::vector<SelectedItem>& items,
                            HWND owner)
{
    size_t chars = 1;
    for (const SelectedItem& item : items) {
        chars += item.path.size() + 1;
    }
    const unsigned long long bytes = static_cast<unsigned long long>(chars) * sizeof(wchar_t);
    SECURITY_ATTRIBUTES inheritable = {sizeof(inheritable), nullptr, TRUE};
    HANDLE memory = CreateFileMappingW(INVALID_HANDLE_VALUE, &inheritable, PAGE_READWRITE, static_cast<DWORD>(bytes >> 32),
                                       static_cast<DWORD>(bytes & 0xFFFFFFFFull), nullptr);
    if (memory == nullptr) {
        return ShowStartError(exe, L"Creating shared memory for the paths failed: ", GetLastError(), owner);
    }
    auto* view = static_cast<wchar_t*>(MapViewOfFile(memory, FILE_MAP_WRITE, 0, 0, 0));
    if (view == nullptr) {
        const DWORD code = GetLastError();
        CloseHandle(memory);
        return ShowStartError(exe, L"Writing the paths to shared memory failed: ", code, owner);
    }
    size_t at = 0;
    for (const SelectedItem& item : items) {
        std::memcpy(view + at, item.path.data(), item.path.size() * sizeof(wchar_t));
        at += item.path.size();
        view[at++] = L'\0';
    }
    view[at] = L'\0';
    UnmapViewOfFile(view);
    AppendArg(commandLine, L"--paths-memory");
    AppendArg(commandLine, std::to_wstring(reinterpret_cast<UINT_PTR>(memory)));
    const HRESULT result = StartProcess(exe, commandLine, memory, owner);
    // The worker has its own handle, and the memory stays until the worker closes it.
    CloseHandle(memory);
    return result;
}

ArchiveExt OtherExt(ArchiveExt ext)
{
    return ext == ArchiveExt::Tgz ? ArchiveExt::TarGz : ArchiveExt::Tgz;
}

// Worker options of each archive command, written before the paths.
std::vector<std::wstring> WorkerOptions(MenuCommand command, const Settings& settings)
{
    switch (command) {
    case MenuCommand::AddOne:
        return {L"--add", L"--one", L"--ext", ExtLabel(settings.ext), L"--level", std::to_wstring(settings.level)};
    case MenuCommand::Add:
        return {L"--add", L"--ext", ExtLabel(settings.ext), L"--level", std::to_wstring(settings.level)};
    case MenuCommand::AddOther:
        return {L"--add", L"--ext", ExtLabel(OtherExt(settings.ext)), L"--level", std::to_wstring(settings.level)};
    case MenuCommand::ExtractHere:
        return {L"--extract", L"--here"};
    case MenuCommand::ExtractTo:
        return {L"--extract", L"--pick-dest"};
    default:
        return {L"--extract"};
    }
}

}  // namespace

HRESULT LaunchCommand(MenuCommand command, const Settings& settings, const std::vector<SelectedItem>& items,
                      HWND owner)
{
    const std::wstring folder = ModuleFolder(Module());
    if (command == MenuCommand::Settings) {
        const std::wstring tray = JoinPath(folder, L"AddToTarGzTray.exe");
        std::wstring commandLine;
        AppendArg(commandLine, tray);
        AppendArg(commandLine, L"--settings");
        return StartProcess(tray, commandLine, nullptr, owner);
    }
    const std::wstring exe = JoinPath(folder, L"AddToTarGz.exe");
    std::wstring prefix;
    AppendArg(prefix, exe);
    for (const std::wstring& option : WorkerOptions(command, settings)) {
        AppendArg(prefix, option);
    }
    if (command == MenuCommand::AddOne) {
        return StartWithPathMemory(exe, prefix, items, owner);
    }
    AppendArg(prefix, L"--");

    std::wstring commandLine = prefix;
    size_t pathsInBatch = 0;
    for (const SelectedItem& item : items) {
        std::wstring quoted;
        AppendArg(quoted, item.path);
        if (pathsInBatch > 0 && commandLine.size() + 1 + quoted.size() > kCommandLineBudget) {
            const HRESULT result = StartProcess(exe, commandLine, nullptr, owner);
            if (FAILED(result)) {
                return result;
            }
            commandLine = prefix;
            pathsInBatch = 0;
        }
        commandLine += L" " + quoted;
        ++pathsInBatch;
    }
    return StartProcess(exe, commandLine, nullptr, owner);
}
