// job_result.h: outcome of one add or extract job, as written to the report.
// Globals: JobStatus, JobResult.
// Doc: docs/notes/src/worker/job_result.md
#pragma once
#include <string>

enum class JobStatus { Done, DoneWithWarnings, Failed, Cancelled, NotStarted };

struct JobResult {
    JobStatus status = JobStatus::Failed;
    std::wstring input;   // the path that was picked
    std::wstring output;  // the finished archive or folder, or the .partial one when the job did not finish
    std::wstring detail;  // exact messages: Windows error text, or 7-Zip exit codes and output
};
