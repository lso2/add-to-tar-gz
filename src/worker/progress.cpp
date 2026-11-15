// progress.cpp: state shared by the job thread and the progress window.
// Globals: none beyond progress.h.
// Doc: docs/notes/src/worker/progress.md
#include "progress.h"

void Progress::BeginItem(size_t index, size_t count, const std::wstring& line, uint64_t totalBytes)
{
    std::lock_guard<std::mutex> lock(mutex_);
    view_.line = line;
    view_.index = index;
    view_.count = count;
    view_.totalBytes = totalBytes;
    doneBytes = 0;
}

Progress::View Progress::Snapshot()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return view_;
}
