// stages.h: runs two 7-Zip processes joined by pipes, with the byte relay that feeds the progress bar.
// Add:     7-Zip tar (item -> stdout) -> relay -> 7-Zip gzip (stdin -> output file).
// Extract: archive file -> relay -> 7-Zip gunzip (stdin -> stdout) -> 7-Zip untar (stdin -> folder).
// Globals: StageOutcome, ChainOutcome, RunAddChain, RunExtractChain, DescribeChain.
// Doc: docs/notes/src/worker/stages.md
#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "progress.h"

struct StageOutcome {
    bool started = false;
    std::wstring startError;
    DWORD exitCode = 0;
    std::wstring messages;  // the text 7-Zip wrote to standard error
};

struct ChainOutcome {
    StageOutcome first;
    StageOutcome second;
    DWORD relayError = 0;
    bool cancelled = false;

    // Highest exit code of the two stages, or 255 for a stage that did not start.
    DWORD WorstExitCode() const;
};

// Every path in the stage arguments is absolute, and the stages inherit this process's working folder.
ChainOutcome RunAddChain(const std::wstring& sevenZip, const std::vector<std::wstring>& tarArgs,
                         const std::vector<std::wstring>& gzipArgs, HANDLE outputFile, Progress& progress);

ChainOutcome RunExtractChain(const std::wstring& sevenZip, HANDLE archiveFile, const std::vector<std::wstring>& gunzipArgs,
                             const std::vector<std::wstring>& untarArgs, Progress& progress);

// Report text: each stage's exit code and messages, plus the relay error when there was one.
std::wstring DescribeChain(const ChainOutcome& outcome, const wchar_t* firstName, const wchar_t* secondName);
