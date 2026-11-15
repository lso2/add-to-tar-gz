// extract_job.h: one extract job: an archive becomes a new folder named after it, or its entries go straight
// into a folder (Extract here). Nothing that exists is ever overwritten.
// Globals: ExtractPlacement, RunExtractJob.
// Doc: docs/notes/src/worker/extract_job.md
#pragma once
#include <string>
#include "job_result.h"
#include "progress.h"

struct ExtractPlacement {
    bool here = false;    // the entries go straight into the target folder, each under a free name
    std::wstring folder;  // the target folder, or empty for the archive's own folder
};

JobResult RunExtractJob(const std::wstring& sevenZip, const std::wstring& archive, const ExtractPlacement& placement,
                        size_t index, size_t count, Progress& progress);
