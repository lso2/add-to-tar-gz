// report.cpp: writes the job results to standard output and shows the dialog for jobs that need attention.
// Globals: none beyond report.h.
// Doc: docs/notes/src/worker/report.md
#include "report.h"
#include "names.h"
#include <windows.h>

namespace {

constexpr size_t kDialogTextLimit = 6000;

void WriteStdout(const std::wstring& text)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out == nullptr || out == INVALID_HANDLE_VALUE || text.empty()) {
        return;
    }
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0,
                                          nullptr, nullptr);
    std::string utf8(static_cast<size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data(), bytes, nullptr, nullptr);
    DWORD written = 0;
    WriteFile(out, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
}

const wchar_t* StatusWord(JobStatus status)
{
    switch (status) {
    case JobStatus::Done:
        return L"DONE";
    case JobStatus::DoneWithWarnings:
        return L"WARNINGS";
    case JobStatus::Cancelled:
        return L"CANCELLED";
    case JobStatus::NotStarted:
        return L"NOT STARTED";
    default:
        return L"FAILED";
    }
}

const wchar_t* StatusPhrase(JobStatus status)
{
    switch (status) {
    case JobStatus::DoneWithWarnings:
        return L"finished with warnings";
    case JobStatus::Cancelled:
        return L"was canceled";
    case JobStatus::NotStarted:
        return L"did not start";
    default:
        return L"failed";
    }
}

std::wstring Indented(const std::wstring& text)
{
    std::wstring out = L"  ";
    for (wchar_t c : text) {
        out.push_back(c);
        if (c == L'\n') {
            out += L"  ";
        }
    }
    return out;
}

}  // namespace

int ReportResults(const std::vector<JobResult>& results, Mode mode, bool silent)
{
    size_t done = 0;
    bool anyFailure = false;
    std::wstring lines;
    std::wstring dialog;
    for (const JobResult& result : results) {
        lines += std::wstring(StatusWord(result.status)) + L"\t" + result.input + L"\t" + result.output + L"\r\n";
        if (!result.detail.empty()) {
            lines += Indented(result.detail) + L"\r\n";
        }
        if (result.status == JobStatus::Done) {
            ++done;
            continue;
        }
        anyFailure = anyFailure || result.status == JobStatus::Failed;
        const wchar_t* verb = mode == Mode::Add ? L"Adding " : L"Extracting ";
        dialog += std::wstring(verb) + LeafName(result.input) + L" " + StatusPhrase(result.status) + L".\r\n";
        if (!result.detail.empty()) {
            dialog += result.detail + L"\r\n";
        }
        // The detail of an unfinished job already has its .partial path.
        if (!result.output.empty() && result.detail.find(result.output) == std::wstring::npos) {
            dialog += L"Output: " + result.output + L"\r\n";
        }
        dialog += L"\r\n";
    }
    WriteStdout(lines);
    if (done == results.size()) {
        return 0;
    }
    if (!silent) {
        std::wstring text;
        if (results.size() > 1) {
            text = std::to_wstring(done) + L" of " + std::to_wstring(results.size()) +
                   (mode == Mode::Add ? L" archives were created.\r\n\r\n" : L" archives were extracted.\r\n\r\n");
        }
        text += dialog;
        if (text.size() > kDialogTextLimit) {
            const size_t dropped = text.size() - kDialogTextLimit;
            text.resize(kDialogTextLimit);
            text += L"\r\n[" + std::to_wstring(dropped) + L" more characters not shown here]";
        }
        MessageBoxW(nullptr, text.c_str(), L"AddToTarGz",
                    MB_OK | MB_SETFOREGROUND | (anyFailure ? MB_ICONERROR : MB_ICONWARNING));
    }
    return 1;
}

int ReportUsageError(const std::wstring& error, bool silent)
{
    WriteStdout(L"USAGE ERROR\t" + error + L"\r\n" + UsageText() + L"\r\n");
    if (!silent) {
        const std::wstring text = error + L"\r\n\r\n" + UsageText();
        MessageBoxW(nullptr, text.c_str(), L"AddToTarGz", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    }
    return 2;
}
