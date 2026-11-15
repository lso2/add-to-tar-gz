// main.cpp: entry point of AddToTarGz.exe. Parses the command line, runs every job on a worker thread
// while the progress window runs on this one, then writes the report.
// Globals: wWinMain.
// Doc: docs/notes/src/worker/main.md
#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <thread>
#include <vector>
#include "add_job.h"
#include "args.h"
#include "errors.h"
#include "extract_job.h"
#include "folder_picker.h"
#include "names.h"
#include "paths.h"
#include "progress.h"
#include "progress_window.h"
#include "report.h"
#include "sevenzip.h"

namespace {

// One result per path with the same status and text, for a run that ends before its first job.
std::vector<JobResult> SameResultForAll(const Options& options, JobStatus status, const std::wstring& detail)
{
    std::vector<JobResult> results;
    for (const std::wstring& path : options.paths) {
        JobResult result;
        result.status = status;
        result.input = path;
        result.detail = detail;
        results.push_back(result);
    }
    return results;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    Options options;
    std::wstring error;
    if (!ParseArgs(options, error)) {
        return ReportUsageError(error, options.silent);
    }
    INITCOMMONCONTROLSEX controls = {sizeof(controls), ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);

    if (options.pickDest) {
        // The folder dialog is a COM object and needs an apartment on this thread.
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        std::wstring picked;
        const HRESULT picking = PickFolder(ParentFolder(options.paths.front()), picked);
        if (picking == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
            // Closing the dialog is the user's choice, and the run ends with no report dialog.
            return ReportResults(SameResultForAll(options, JobStatus::NotStarted,
                                                  L"The Extract to dialog was closed without a folder."),
                                 options.mode, true);
        }
        if (FAILED(picking)) {
            return ReportResults(SameResultForAll(options, JobStatus::Failed,
                                                  L"The Extract to dialog failed: " +
                                                      WindowsErrorText(static_cast<DWORD>(picking))),
                                 options.mode, options.silent);
        }
        options.dest = picked;
    }

    const std::wstring sevenZip = SevenZipPath();
    const bool haveSevenZip = PathExists(sevenZip) && !IsFolder(sevenZip);
    ExtractPlacement placement;
    placement.here = options.here;
    placement.folder = options.dest;
    Progress progress;
    std::vector<JobResult> results;
    std::thread jobs([&] {
        if (options.one) {
            if (haveSevenZip) {
                results.push_back(RunAddOneJob(sevenZip, options.paths, options.ext, options.level, progress));
            } else {
                JobResult missing;
                missing.input = ParentFolder(options.paths.front());
                missing.detail = L"7za.exe was not found at " + sevenZip + L".";
                results.push_back(missing);
            }
            progress.finished = true;
            return;
        }
        const size_t count = options.paths.size();
        for (size_t i = 0; i < count; ++i) {
            const std::wstring& path = options.paths[i];
            if (!haveSevenZip) {
                JobResult missing;
                missing.input = path;
                missing.detail = L"7za.exe was not found at " + sevenZip + L".";
                results.push_back(missing);
                continue;
            }
            if (progress.cancel.load()) {
                JobResult skipped;
                skipped.status = JobStatus::NotStarted;
                skipped.input = path;
                skipped.detail = L"The run was canceled before this item.";
                results.push_back(skipped);
                continue;
            }
            results.push_back(options.mode == Mode::Add
                                  ? RunAddJob(sevenZip, path, options.ext, options.level, i, count, progress)
                                  : RunExtractJob(sevenZip, path, placement, i, count, progress));
        }
        progress.finished = true;
    });
    if (!options.silent) {
        RunProgressWindow(progress);
    }
    jobs.join();
    return ReportResults(results, options.mode, options.silent);
}
