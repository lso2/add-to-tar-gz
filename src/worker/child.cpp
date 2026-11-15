// child.cpp: starts a hidden child process and collects its standard error.
// Globals: none beyond child.h.
// Doc: docs/notes/src/worker/child.md
#include "child.h"
#include "cmdline.h"
#include "errors.h"
#include "pipes.h"

namespace {

constexpr size_t kErrorBytesKept = 48 * 1024;

std::wstring FromUtf8(const std::string& bytes)
{
    if (bytes.empty()) {
        return std::wstring();
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
    std::wstring text(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, bytes.data(), static_cast<int>(bytes.size()), text.data(), length);
    return text;
}

}  // namespace

ChildProcess::~ChildProcess()
{
    if (reader_.joinable()) {
        reader_.join();
    }
    CloseIfOpen(errorRead_);
    CloseIfOpen(process_);
}

bool ChildProcess::Start(const std::wstring& exe, const std::vector<std::wstring>& args, const std::wstring& cwd,
                         HANDLE stdinHandle, HANDLE stdoutHandle, std::wstring& error)
{
    Pipe errorPipe;
    if (!MakePipe(errorPipe, false, true, error)) {
        return false;
    }

    // The child inherits the listed handles and no others. A stray pipe end inherited by the wrong stage
    // holds that pipe open, and its reader waits forever for end of input.
    HANDLE inherited[3] = {};
    DWORD inheritedCount = 0;
    for (HANDLE handle : {stdinHandle, stdoutHandle, errorPipe.write}) {
        bool listed = handle == nullptr;
        for (DWORD i = 0; i < inheritedCount; ++i) {
            listed = listed || inherited[i] == handle;
        }
        if (!listed) {
            inherited[inheritedCount++] = handle;
        }
    }
    SIZE_T listSize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &listSize);
    std::vector<char> listMemory(listSize);
    auto list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(listMemory.data());
    if (!InitializeProcThreadAttributeList(list, 1, 0, &listSize) ||
        !UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited,
                                   inheritedCount * sizeof(HANDLE), nullptr, nullptr)) {
        error = L"Preparing the process attributes failed: " + WindowsErrorText(GetLastError());
        CloseIfOpen(errorPipe.read);
        CloseIfOpen(errorPipe.write);
        return false;
    }

    STARTUPINFOEXW startup = {};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.StartupInfo.wShowWindow = SW_HIDE;
    startup.StartupInfo.hStdInput = stdinHandle;
    startup.StartupInfo.hStdOutput = stdoutHandle;
    startup.StartupInfo.hStdError = errorPipe.write;
    startup.lpAttributeList = list;

    std::wstring commandLine;
    AppendArg(commandLine, exe);
    for (const std::wstring& arg : args) {
        AppendArg(commandLine, arg);
    }

    PROCESS_INFORMATION info = {};
    const BOOL started = CreateProcessW(exe.c_str(), commandLine.data(), nullptr, nullptr, TRUE,
                                       CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr,
                                       cwd.empty() ? nullptr : cwd.c_str(), &startup.StartupInfo, &info);
    const DWORD startError = GetLastError();
    DeleteProcThreadAttributeList(list);
    CloseIfOpen(errorPipe.write);
    if (!started) {
        error = L"Starting " + exe + L" failed: " + WindowsErrorText(startError);
        CloseIfOpen(errorPipe.read);
        return false;
    }
    CloseHandle(info.hThread);
    process_ = info.hProcess;
    errorRead_ = errorPipe.read;
    reader_ = std::thread([this] {
        char chunk[4096];
        DWORD got = 0;
        while (ReadFile(errorRead_, chunk, sizeof(chunk), &got, nullptr) && got > 0) {
            const size_t room = kErrorBytesKept > errorBytes_.size() ? kErrorBytesKept - errorBytes_.size() : 0;
            const size_t keep = got < room ? got : room;
            errorBytes_.append(chunk, keep);
            droppedBytes_ += got - keep;
        }
    });
    return true;
}

void ChildProcess::Terminate()
{
    if (process_ != nullptr && WaitForSingleObject(process_, 0) == WAIT_TIMEOUT) {
        TerminateProcess(process_, 255);
        WaitForSingleObject(process_, 5000);
    }
}

DWORD ChildProcess::ExitCode() const
{
    DWORD code = 0;
    if (process_ == nullptr || !GetExitCodeProcess(process_, &code)) {
        return 255;
    }
    return code;
}

std::wstring ChildProcess::ErrorText()
{
    if (reader_.joinable()) {
        reader_.join();
    }
    std::wstring text = FromUtf8(errorBytes_);
    while (!text.empty() && (text.back() == L'\r' || text.back() == L'\n' || text.back() == L' ')) {
        text.pop_back();
    }
    if (droppedBytes_ > 0) {
        text += L"\r\n[" + std::to_wstring(droppedBytes_) + L" more bytes of 7-Zip output were not kept]";
    }
    return text;
}
