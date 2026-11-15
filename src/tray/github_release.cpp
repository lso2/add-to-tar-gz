// github_release.cpp: tag, version and download address of a GitHub release answer.
// Globals: none beyond github_release.h.
// Doc: docs/notes/src/tray/github_release.md
#include "github_release.h"
#include "json_reader.h"
#include "links.h"
#include "release_version.h"
#include <windows.h>
#include <cwchar>

namespace {

bool StartsWithNoCase(const std::wstring& text, const wchar_t* prefix)
{
    const size_t length = wcslen(prefix);
    return text.size() >= length && CompareStringOrdinal(text.c_str(), static_cast<int>(length), prefix,
                                                         static_cast<int>(length), TRUE) == CSTR_EQUAL;
}

bool EndsWithNoCase(const std::wstring& text, const wchar_t* suffix)
{
    const size_t length = wcslen(suffix);
    return text.size() >= length &&
           CompareStringOrdinal(text.c_str() + (text.size() - length), static_cast<int>(length), suffix,
                                static_cast<int>(length), TRUE) == CSTR_EQUAL;
}

// Only https addresses from a release are opened. ShellExecuteW runs a local program for a path.
bool IsWebAddress(const std::wstring& url)
{
    return StartsWithNoCase(url, L"https://");
}

// The first asset whose name ends with the extension.
std::wstring AssetUrl(const JsonValue& release, const wchar_t* extension)
{
    const JsonValue* assets = release.Member(L"assets");
    if (assets == nullptr || assets->type != JsonValue::Type::Array) {
        return std::wstring();
    }
    for (const JsonValue& asset : assets->items) {
        const std::wstring url = asset.MemberText(L"browser_download_url");
        if (EndsWithNoCase(asset.MemberText(L"name"), extension) && IsWebAddress(url)) {
            return url;
        }
    }
    return std::wstring();
}

// The order of QuickPane's PickDownload: the installer, then a zip, then the release's page.
std::wstring PickDownload(const JsonValue& release)
{
    std::wstring url = AssetUrl(release, L".exe");
    if (url.empty()) {
        url = AssetUrl(release, L".zip");
    }
    if (url.empty()) {
        url = release.MemberText(L"html_url");
        if (!IsWebAddress(url)) {
            url.clear();
        }
    }
    return url.empty() ? std::wstring(kReleasesPage) : url;
}

}  // namespace

UpdateResult ReadLatestRelease(const std::string& json)
{
    UpdateResult result;
    JsonValue release;
    std::wstring parseError;
    if (!ParseJson(json, release, parseError)) {
        result.error = L"GitHub's answer is not valid JSON: " + parseError;
        return result;
    }
    const std::wstring tag = release.MemberText(L"tag_name");
    if (tag.empty()) {
        result.error = L"GitHub returned no release";
        return result;
    }
    ReleaseVersion latest = {};
    if (!ParseReleaseTag(tag, latest)) {
        result.error = L"could not read the release tag \"" + tag + L"\"";
        return result;
    }
    result.latest = VersionText(latest);
    result.available = CompareVersions(latest, CurrentVersion()) > 0;
    result.downloadUrl = PickDownload(release);
    return result;
}

std::wstring ReadErrorMessage(const std::string& json)
{
    JsonValue answer;
    std::wstring ignored;
    return ParseJson(json, answer, ignored) ? answer.MemberText(L"message") : std::wstring();
}
