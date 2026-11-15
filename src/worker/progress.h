// progress.h: state shared by the job thread and the progress window.
// Globals: Progress.
// Doc: docs/notes/src/worker/progress.md
#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

class Progress {
public:
    struct View {
        std::wstring line;
        size_t index = 0;
        size_t count = 0;
        uint64_t totalBytes = 0;
    };

    // Resets the byte counter. Called by the job thread for each new item.
    void BeginItem(size_t index, size_t count, const std::wstring& line, uint64_t totalBytes);

    // Copy of the current item description for the window.
    View Snapshot();

    std::atomic<uint64_t> doneBytes{0};
    std::atomic<bool> cancel{false};
    std::atomic<bool> finished{false};

private:
    std::mutex mutex_;
    View view_;
};
