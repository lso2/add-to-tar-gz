// extract_job.cpp: extracts one .tar.gz or .tgz into a new folder, or straight into a folder entry by entry,
// never into or over anything that exists.
// Globals: none beyond extract_job.h.
// Doc: docs/notes/src/worker/extract_job.md
#include "extract_job.h"
#include "errors.h"
#include "fs_ops.h"
#include "names.h"
#include "paths.h"
#include "pipes.h"
#include "sevenzip.h"
#include "stages.h"
#include <windows.h>

namespace {

bool SameNameNoCase(const std::wstring& a, const std::wstring& b)
{
    return CompareStringOrdinal(a.c_str(), static_cast<int>(a.size()), b.c_str(), static_cast<int>(b.size()), TRUE) ==
           CSTR_EQUAL;
}

// Creates "<final>.partial" as a new folder. CreateDirectoryW fails on an existing folder, and no folder is ever reused.
bool ReservePartialFolder(const std::wstring& folder, const std::wstring& stem, std::wstring& finalPath,
                          std::wstring& error)
{
    for (int attempt = 0; attempt < 100; ++attempt) {
        finalPath = FirstFreeName(folder, stem, L"");
        if (finalPath.empty()) {
            error = L"Every folder name from " + stem + L" to " + stem + L" (9999) is taken.";
            return false;
        }
        const std::wstring partial = finalPath + kPartialSuffix;
        if (CreateDirectoryW(ExtendedPath(partial).c_str(), nullptr)) {
            return true;
        }
        const DWORD code = GetLastError();
        if (code != ERROR_ALREADY_EXISTS) {
            error = L"Creating " + partial + L" failed: " + WindowsErrorText(code);
            return false;
        }
    }
    error = L"The next free folder name for " + stem + L" was taken by another program 100 times in a row.";
    return false;
}

void RemoveEmptyPartial(const std::wstring& partialPath, std::wstring& warnings)
{
    // RemoveDirectoryW fails on a folder with any entry left, and this one had only entries moved out of it.
    if (!RemoveDirectoryW(ExtendedPath(partialPath).c_str())) {
        warnings += L"The empty folder " + partialPath + L" was not removed: " + WindowsErrorText(GetLastError()) +
                    L"\r\n";
    }
}

// Extract here: each entry of the .partial folder goes into `target` under its own name, or under the next free
// name when that one is taken. A file has its extension after the number: "readme (2).txt".
bool MoveEntriesInto(const std::wstring& partialPath, const std::wstring& target, std::wstring& output,
                     std::wstring& warnings, std::wstring& error)
{
    const std::vector<ChildEntry> entries = ListChildren(partialPath, static_cast<size_t>(-1));
    for (const ChildEntry& entry : entries) {
        const std::wstring base = ItemBaseName(entry.name, entry.isFolder);
        const std::wstring suffix = entry.name.substr(base.size());
        const std::wstring reserved = FirstFreeName(target, base, suffix, partialPath);
        if (reserved.empty()) {
            error = L"Every name from " + entry.name + L" to " + base + L" (9999)" + suffix + L" is taken in " +
                    target + L". The remaining entries are in " + partialPath + L".";
            return false;
        }
        const std::wstring moved =
            MoveToFreeName(JoinPath(partialPath, entry.name), reserved, target, base, suffix, error, partialPath);
        if (moved.empty()) {
            error += L" The remaining entries are in " + partialPath + L".";
            return false;
        }
        output = entries.size() == 1 ? moved : target;
    }
    if (entries.empty()) {
        output = target;
    }
    RemoveEmptyPartial(partialPath, warnings);
    return true;
}

// Extract to "name\": the .partial folder becomes the final folder. A lone root folder named like the archive is
// renamed to the final folder itself, the folder the menu label shows.
bool FinishNamedFolder(const std::wstring& partialPath, const std::wstring& finalPath, const std::wstring& target,
                       const std::wstring& stem, std::wstring& output, std::wstring& warnings, std::wstring& error)
{
    const std::vector<ChildEntry> entries = ListChildren(partialPath, 2);
    if (entries.size() == 1 && entries[0].isFolder && !entries[0].isReparsePoint && SameNameNoCase(entries[0].name, stem)) {
        output = MoveToFreeName(JoinPath(partialPath, entries[0].name), finalPath, target, stem, L"", error);
        if (!output.empty()) {
            RemoveEmptyPartial(partialPath, warnings);
        }
    } else {
        output = MoveToFreeName(partialPath, finalPath, target, stem, L"", error);
    }
    return !output.empty();
}

}  // namespace

JobResult RunExtractJob(const std::wstring& sevenZip, const std::wstring& archive, const ExtractPlacement& placement,
                        size_t index, size_t count, Progress& progress)
{
    JobResult result;
    result.input = archive;

    const std::wstring target = placement.folder.empty() ? ParentFolder(archive) : placement.folder;
    const std::wstring leaf = LeafName(archive);
    if (!IsArchiveFileName(leaf)) {
        result.detail = leaf + L" does not end in .tar.gz or .tgz.";
        return result;
    }
    HANDLE source = CreateFileW(ExtendedPath(archive).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (source == INVALID_HANDLE_VALUE) {
        result.detail = L"Opening " + archive + L" failed: " + WindowsErrorText(GetLastError());
        return result;
    }
    LARGE_INTEGER size = {};
    GetFileSizeEx(source, &size);

    const std::wstring stem = ArchiveStem(leaf);
    std::wstring finalPath;
    if (!ReservePartialFolder(target, stem, finalPath, result.detail)) {
        CloseIfOpen(source);
        return result;
    }
    const std::wstring partialPath = finalPath + kPartialSuffix;
    result.output = partialPath;

    progress.BeginItem(index, count, L"Extracting " + leaf, static_cast<uint64_t>(size.QuadPart));
    const ChainOutcome chain =
        RunExtractChain(sevenZip, source, GzipReadArgs(), TarExtractArgs(partialPath), progress);
    CloseIfOpen(source);

    if (chain.cancelled) {
        result.status = JobStatus::Cancelled;
        result.detail = L"The incomplete folder is " + partialPath + L".";
        return result;
    }
    const DWORD worst = chain.WorstExitCode();
    if (worst >= 2) {
        result.detail = DescribeChain(chain, L"gunzip", L"untar") + L"The incomplete folder is " + partialPath + L".";
        return result;
    }

    std::wstring warnings;
    std::wstring moveError;
    std::wstring finished;
    const bool moved = placement.here
                           ? MoveEntriesInto(partialPath, target, finished, warnings, moveError)
                           : FinishNamedFolder(partialPath, finalPath, target, stem, finished, warnings, moveError);
    if (!moved) {
        result.detail = DescribeChain(chain, L"gunzip", L"untar") + moveError;
        return result;
    }
    result.output = finished;
    if (worst == 1 || chain.relayError != 0 || !warnings.empty()) {
        result.status = JobStatus::DoneWithWarnings;
        result.detail = DescribeChain(chain, L"gunzip", L"untar") + warnings;
    } else {
        result.status = JobStatus::Done;
    }
    return result;
}
