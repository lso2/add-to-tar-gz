// add_job.h: add jobs, each writing one new archive beside its items.
// Globals: RunAddJob, RunAddOneJob.
// Doc: docs/notes/src/worker/add_job.md
#pragma once
#include <string>
#include <vector>
#include "job_result.h"
#include "names.h"
#include "progress.h"

JobResult RunAddJob(const std::wstring& sevenZip, const std::wstring& item, ArchiveExt ext, int level, size_t index,
                    size_t count, Progress& progress);

// Every item in one new archive named after the first item's folder (FolderBaseName), in that folder.
JobResult RunAddOneJob(const std::wstring& sevenZip, const std::vector<std::wstring>& items, ArchiveExt ext, int level,
                       Progress& progress);
