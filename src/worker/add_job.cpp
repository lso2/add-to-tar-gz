// add_job.cpp: add jobs, each writing one new archive beside its items without touching an existing file.
// Globals: none beyond add_job.h.
// Doc: docs/notes/src/worker/add_job.md
#include "add_job.h"
#include "errors.h"
#include "fs_ops.h"
#include "paths.h"
#include "pipes.h"
#include "sevenzip.h"
#include "stages.h"
#include <windows.h>

namespace {

// Creates "<final>.partial" with CREATE_NEW, and from that moment on the name belongs to this job.
HANDLE ReservePartial(const std::wstring& folder, const std::wstring& base, const std::wstring& suffix,
                      std::wstring& finalPath, std::wstring& error)
{
    for (int attempt = 0; attempt < 100; ++attempt) {
        finalPath = FirstFreeName(folder, base, suffix);
        if (finalPath.empty()) {
            error = L"Every name from " + base + suffix + L" to " + base + L" (9999)" + suffix + L" is taken.";
            return INVALID_HANDLE_VALUE;
        }
        const std::wstring partial = finalPath + kPartialSuffix;
        HANDLE file = CreateFileW(ExtendedPath(partial).c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW,
                                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        if (file != INVALID_HANDLE_VALUE) {
            return file;
        }
        const DWORD code = GetLastError();
        if (code != ERROR_FILE_EXISTS && code != ERROR_ALREADY_EXISTS) {
            error = L"Creating " + partial + L" failed: " + WindowsErrorText(code);
            return INVALID_HANDLE_VALUE;
        }
    }
    error = L"The next free name for " + base + suffix + L" was taken by another program 100 times in a row.";
    return INVALID_HANDLE_VALUE;
}

// The job's outcome from its chain. A finished archive gets its final name, and an unfinished one keeps its
// .partial name, given in the detail.
void FinishJob(const ChainOutcome& chain, const std::wstring& finalPath, const std::wstring& folder,
               const std::wstring& base, const std::wstring& suffix, JobResult& result)
{
    const std::wstring partialPath = finalPath + kPartialSuffix;
    if (chain.cancelled) {
        result.status = JobStatus::Cancelled;
        result.detail = L"The incomplete archive is " + partialPath + L".";
        return;
    }
    const DWORD worst = chain.WorstExitCode();
    // 7-Zip exit code 1 is a warning, such as a file another program had locked, and 2 and above are errors.
    if (worst >= 2) {
        result.detail = DescribeChain(chain, L"tar", L"gzip") + L"The incomplete archive is " + partialPath + L".";
        return;
    }
    std::wstring renameError;
    const std::wstring finished = MoveToFreeName(partialPath, finalPath, folder, base, suffix, renameError);
    if (finished.empty()) {
        result.detail = DescribeChain(chain, L"tar", L"gzip") + renameError;
        return;
    }
    result.output = finished;
    if (worst == 1 || chain.relayError != 0) {
        result.status = JobStatus::DoneWithWarnings;
        result.detail = DescribeChain(chain, L"tar", L"gzip");
    } else {
        result.status = JobStatus::Done;
    }
}

}  // namespace

JobResult RunAddJob(const std::wstring& sevenZip, const std::wstring& item, ArchiveExt ext, int level, size_t index,
                    size_t count, Progress& progress)
{
    JobResult result;
    result.input = item;

    const DWORD attributes = GetFileAttributesW(ExtendedPath(item).c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        result.detail = L"Reading " + item + L" failed: " + WindowsErrorText(GetLastError());
        return result;
    }
    const std::wstring folder = ParentFolder(item);
    const std::wstring leaf = LeafName(item);
    if (folder.empty() || IsRootPath(item)) {
        result.detail = item + L" is a drive or share root. A root has no parent folder to hold the archive.";
        return result;
    }
    const bool isFolder = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    const std::wstring base = ItemBaseName(leaf, isFolder);
    const std::wstring suffix = ExtSuffix(ext);

    std::wstring finalPath;
    HANDLE output = ReservePartial(folder, base, suffix, finalPath, result.detail);
    if (output == INVALID_HANDLE_VALUE) {
        return result;
    }
    const std::wstring partialPath = finalPath + kPartialSuffix;
    result.output = partialPath;

    const uint64_t total = isFolder ? FolderSize(item, progress.cancel) : FileSize(item);
    progress.BeginItem(index, count, L"Compressing " + leaf, total);
    const ChainOutcome chain =
        RunAddChain(sevenZip, TarCreateArgs(item), GzipCreateArgs(base + L".tar", level), output, progress);
    CloseIfOpen(output);
    FinishJob(chain, finalPath, folder, base, suffix, result);
    return result;
}

JobResult RunAddOneJob(const std::wstring& sevenZip, const std::vector<std::wstring>& items, ArchiveExt ext, int level,
                       Progress& progress)
{
    JobResult result;
    const std::wstring folder = ParentFolder(items.front());
    result.input = folder;
    std::vector<bool> isFolder;
    for (const std::wstring& item : items) {
        const DWORD attributes = GetFileAttributesW(ExtendedPath(item).c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            result.detail = L"Reading " + item + L" failed: " + WindowsErrorText(GetLastError());
            return result;
        }
        if (IsRootPath(item)) {
            result.detail = item + L" is a drive or share root. A root has no parent folder to hold the archive.";
            return result;
        }
        isFolder.push_back((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
    }
    if (folder.empty()) {
        result.detail = items.front() + L" has no parent folder to hold the archive.";
        return result;
    }
    const std::wstring base = FolderBaseName(folder);
    const std::wstring suffix = ExtSuffix(ext);

    std::wstring listPath;
    if (!WriteListFile(items, listPath, result.detail)) {
        return result;
    }
    std::wstring finalPath;
    HANDLE output = ReservePartial(folder, base, suffix, finalPath, result.detail);
    if (output == INVALID_HANDLE_VALUE) {
        DeleteFileW(listPath.c_str());
        return result;
    }
    result.output = finalPath + kPartialSuffix;

    uint64_t total = 0;
    for (size_t i = 0; i < items.size(); ++i) {
        total += isFolder[i] ? FolderSize(items[i], progress.cancel) : FileSize(items[i]);
    }
    const std::wstring counted = items.size() == 1 ? std::wstring(L"1 item") : std::to_wstring(items.size()) + L" items";
    progress.BeginItem(0, 1, L"Compressing " + counted + L" into " + LeafName(finalPath), total);
    const ChainOutcome chain =
        RunAddChain(sevenZip, TarListArgs(listPath), GzipCreateArgs(base + L".tar", level), output, progress);
    CloseIfOpen(output);
    DeleteFileW(listPath.c_str());
    FinishJob(chain, finalPath, folder, base, suffix, result);
    return result;
}
