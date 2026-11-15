// report.h: end-of-run report. One line per job is written to standard output, and one dialog appears for a job
// that ended with warnings, failed, was canceled or did not start.
// Globals: ReportResults, ReportUsageError.
// Doc: docs/notes/src/worker/report.md
#pragma once
#include <string>
#include <vector>
#include "args.h"
#include "job_result.h"

// Returns the process exit code: 0 when every job is Done, 1 otherwise.
int ReportResults(const std::vector<JobResult>& results, Mode mode, bool silent);

// Reports a command line error with the usage text. Returns exit code 2.
int ReportUsageError(const std::wstring& error, bool silent);
