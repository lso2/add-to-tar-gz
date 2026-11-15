// child.h: one hidden child process with chosen standard input and output and a captured standard error.
// Globals: ChildProcess.
// Doc: docs/notes/src/worker/child.md
#pragma once
#include <windows.h>
#include <string>
#include <thread>
#include <vector>

class ChildProcess {
public:
    ChildProcess() = default;
    ~ChildProcess();
    ChildProcess(const ChildProcess&) = delete;
    ChildProcess& operator=(const ChildProcess&) = delete;

    // Starts exe with args in cwd. stdinHandle and stdoutHandle must be inheritable, as the only
    // handles the child inherits besides its own standard error pipe.
    bool Start(const std::wstring& exe, const std::vector<std::wstring>& args, const std::wstring& cwd,
               HANDLE stdinHandle, HANDLE stdoutHandle, std::wstring& error);

    HANDLE Process() const { return process_; }

    // Ends the process when it is still running.
    void Terminate();

    // Exit code once the process has ended.
    DWORD ExitCode() const;

    // Waits for the standard error reader and returns what the child wrote, decoded as UTF-8.
    std::wstring ErrorText();

private:
    HANDLE process_ = nullptr;
    HANDLE errorRead_ = nullptr;
    std::thread reader_;
    std::string errorBytes_;
    size_t droppedBytes_ = 0;
};
