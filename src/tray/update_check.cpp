// update_check.cpp: the WinHTTP request for GitHub's latest release, run on its own thread.
// Globals: none beyond update_check.h.
// Doc: docs/notes/src/tray/update_check.md
#include "update_check.h"
#include "errors.h"
#include "github_release.h"
#include "links.h"
#include "update_result.h"
#include "version.h"
#include <winhttp.h>
#include <memory>

namespace {

// QuickPane's Timeout and ReadWriteTimeout, applied to each WinHTTP phase.
constexpr int kTimeoutMs = 10000;
// A release answer is a few kilobytes. The cap bounds the memory a broken answer can take.
constexpr size_t kMaxAnswerBytes = 4 * 1024 * 1024;

class InternetHandle {
public:
    explicit InternetHandle(HINTERNET handle) : handle_(handle) {}
    ~InternetHandle()
    {
        if (handle_ != nullptr) {
            WinHttpCloseHandle(handle_);
        }
    }
    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;
    HINTERNET get() const { return handle_; }

private:
    HINTERNET handle_;
};

struct Answer {
    DWORD status = 0;
    std::wstring statusText;
    std::string body;
};

struct CheckJob {
    HWND notify = nullptr;
    UINT message = 0;
    std::unique_ptr<UpdateResult> result;
};

// WinHTTP's own codes have their text in winhttp.dll, and the system table has the rest.
std::wstring StepError(const wchar_t* step, DWORD code)
{
    wchar_t* buffer = nullptr;
    const DWORD length = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                                            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                        GetModuleHandleW(L"winhttp.dll"), code, 0, reinterpret_cast<wchar_t*>(&buffer),
                                        0, nullptr);
    std::wstring text;
    if (length != 0 && buffer != nullptr) {
        text.assign(buffer, length);
        while (!text.empty() && (text.back() == L'\r' || text.back() == L'\n' || text.back() == L' ')) {
            text.pop_back();
        }
    }
    if (buffer != nullptr) {
        LocalFree(buffer);
    }
    return std::wstring(step) + L" failed: " + text + L" (error " + std::to_wstring(code) + L")";
}

// The reason phrase after the status code, such as "Not Found". Empty when the answer has none.
std::wstring StatusText(HINTERNET request)
{
    DWORD size = 0;
    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_TEXT, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER,
                        &size, WINHTTP_NO_HEADER_INDEX);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0) {
        return std::wstring();
    }
    std::wstring text(size / sizeof(wchar_t), L'\0');
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_TEXT, WINHTTP_HEADER_NAME_BY_INDEX, text.data(), &size,
                             WINHTTP_NO_HEADER_INDEX)) {
        return std::wstring();
    }
    text.resize(size / sizeof(wchar_t));
    return text;
}

bool ReadBody(HINTERNET request, const std::wstring& host, std::string& body, std::wstring& error)
{
    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            error = StepError(L"WinHttpQueryDataAvailable", GetLastError());
            return false;
        }
        if (available == 0) {
            return true;
        }
        if (available > kMaxAnswerBytes - body.size()) {
            error = L"the answer from " + host + L" is larger than " +
                    std::to_wstring(kMaxAnswerBytes / (1024 * 1024)) + L" MB";
            return false;
        }
        const size_t offset = body.size();
        body.resize(offset + available);
        DWORD read = 0;
        if (!WinHttpReadData(request, &body[offset], available, &read)) {
            error = StepError(L"WinHttpReadData", GetLastError());
            return false;
        }
        body.resize(offset + read);
        if (read == 0) {
            return true;
        }
    }
}

// One GET with QuickPane's headers: the User-Agent GitHub requires of every API call, and GitHub's JSON type.
// The proxy is the one in the Windows proxy settings, as for QuickPane's HttpWebRequest.
bool Fetch(const URL_COMPONENTS& url, Answer& answer, std::wstring& error)
{
    InternetHandle session(WinHttpOpen(L"AddToTarGz/" ATG_VERSION_STRING, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                       WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (session.get() == nullptr) {
        error = StepError(L"WinHttpOpen", GetLastError());
        return false;
    }
    if (!WinHttpSetTimeouts(session.get(), kTimeoutMs, kTimeoutMs, kTimeoutMs, kTimeoutMs)) {
        error = StepError(L"WinHttpSetTimeouts", GetLastError());
        return false;
    }
    const std::wstring host(url.lpszHostName, url.dwHostNameLength);
    InternetHandle connection(WinHttpConnect(session.get(), host.c_str(), url.nPort, 0));
    if (connection.get() == nullptr) {
        error = StepError(L"WinHttpConnect", GetLastError());
        return false;
    }
    const std::wstring path(url.lpszUrlPath, url.dwUrlPathLength);
    LPCWSTR accept[] = {L"application/vnd.github+json", nullptr};
    InternetHandle request(WinHttpOpenRequest(connection.get(), L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                              accept,
                                              url.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0));
    if (request.get() == nullptr) {
        error = StepError(L"WinHttpOpenRequest", GetLastError());
        return false;
    }
    if (!WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        error = StepError(L"WinHttpSendRequest", GetLastError());
        return false;
    }
    if (!WinHttpReceiveResponse(request.get(), nullptr)) {
        error = StepError(L"WinHttpReceiveResponse", GetLastError());
        return false;
    }
    DWORD size = sizeof(answer.status);
    if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &answer.status, &size, WINHTTP_NO_HEADER_INDEX)) {
        error = StepError(L"WinHttpQueryHeaders", GetLastError());
        return false;
    }
    answer.statusText = StatusText(request.get());
    return ReadBody(request.get(), host, answer.body, error);
}

void Check(UpdateResult& result)
{
    wchar_t host[256] = {};
    wchar_t path[1024] = {};
    URL_COMPONENTS url = {};
    url.dwStructSize = sizeof(url);
    url.lpszHostName = host;
    url.dwHostNameLength = ARRAYSIZE(host);
    url.lpszUrlPath = path;
    url.dwUrlPathLength = ARRAYSIZE(path);
    if (!WinHttpCrackUrl(kLatestReleaseApi, 0, 0, &url)) {
        result.error = StepError(L"WinHttpCrackUrl", GetLastError());
        return;
    }
    Answer answer;
    std::wstring error;
    if (!Fetch(url, answer, error)) {
        result.error = error;
        return;
    }
    if (answer.status != HTTP_STATUS_OK) {
        result.error = L"HTTP " + std::to_wstring(answer.status);
        if (!answer.statusText.empty()) {
            result.error += L" " + answer.statusText;
        }
        result.error += L" from " + std::wstring(host);
        // GitHub's error answers carry a message, such as the one for an exceeded rate limit.
        const std::wstring message = ReadErrorMessage(answer.body);
        if (!message.empty() && CompareStringOrdinal(message.c_str(), -1, answer.statusText.c_str(), -1, TRUE) !=
                                    CSTR_EQUAL) {
            result.error += L": " + message;
        }
        return;
    }
    result = ReadLatestRelease(answer.body);
}

DWORD WINAPI CheckThread(void* parameter)
{
    std::unique_ptr<CheckJob> job(static_cast<CheckJob*>(parameter));
    try {
        Check(*job->result);
    } catch (...) {
        // Allocation failures end here. The settings window reads an empty version as a failed check.
        job->result->latest.clear();
        job->result->available = false;
    }
    if (PostMessageW(job->notify, job->message, 0, reinterpret_cast<LPARAM>(job->result.get()))) {
        job->result.release();
    }
    return 0;
}

}  // namespace

bool StartUpdateCheck(HWND notify, UINT message, std::wstring& error)
{
    auto job = std::make_unique<CheckJob>();
    job->notify = notify;
    job->message = message;
    job->result = std::make_unique<UpdateResult>();
    HANDLE thread = CreateThread(nullptr, 0, CheckThread, job.get(), 0, nullptr);
    if (thread == nullptr) {
        error = L"CreateThread failed: " + WindowsErrorText(GetLastError());
        return false;
    }
    // The thread owns the job from here on.
    job.release();
    CloseHandle(thread);
    return true;
}
