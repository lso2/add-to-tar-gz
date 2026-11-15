// release_version.cpp: release tag parsing and version comparison.
// Globals: none beyond release_version.h.
// Doc: docs/notes/src/tray/release_version.md
#include "release_version.h"
#include "version.h"

namespace {

constexpr unsigned long long kMaxNumber = 2147483647;

bool IsDigit(wchar_t c)
{
    return c >= L'0' && c <= L'9';
}

// True when a '.' and a digit follow `pos`.
bool DotDigitAt(const std::wstring& text, size_t pos)
{
    return pos + 1 < text.size() && text[pos] == L'.' && IsDigit(text[pos + 1]);
}

// Reads the digits at `pos` and moves `pos` past them. False when the number is above kMaxNumber.
bool ReadNumber(const std::wstring& text, size_t& pos, unsigned& value)
{
    unsigned long long number = 0;
    bool fits = true;
    while (pos < text.size() && IsDigit(text[pos])) {
        number = number * 10 + static_cast<unsigned>(text[pos] - L'0');
        if (number > kMaxNumber) {
            fits = false;
            number = kMaxNumber;
        }
        ++pos;
    }
    value = static_cast<unsigned>(number);
    return fits;
}

}  // namespace

bool ParseReleaseTag(const std::wstring& tag, ReleaseVersion& version)
{
    for (size_t start = 0; start < tag.size(); ++start) {
        // Only the first digit of a run of digits can begin a match. A start inside the run has the same
        // next character as the run's first digit.
        if (!IsDigit(tag[start]) || (start > 0 && IsDigit(tag[start - 1]))) {
            continue;
        }
        size_t end = start;
        while (end < tag.size() && IsDigit(tag[end])) {
            ++end;
        }
        if (!DotDigitAt(tag, end)) {
            continue;
        }
        ReleaseVersion parsed = {0, 0, 0, 0};
        size_t pos = start;
        bool fits = ReadNumber(tag, pos, parsed[0]);
        for (size_t part = 1; part < parsed.size() && DotDigitAt(tag, pos); ++part) {
            ++pos;
            fits = ReadNumber(tag, pos, parsed[part]) && fits;
        }
        // QuickPane reads only the first match, and a number too large for a version fails the tag.
        if (!fits) {
            return false;
        }
        version = parsed;
        return true;
    }
    return false;
}

int CompareVersions(const ReleaseVersion& a, const ReleaseVersion& b)
{
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            return a[i] < b[i] ? -1 : 1;
        }
    }
    return 0;
}

std::wstring VersionText(const ReleaseVersion& version)
{
    std::wstring text = std::to_wstring(version[0]) + L"." + std::to_wstring(version[1]) + L"." +
                        std::to_wstring(version[2]);
    if (version[3] != 0) {
        text += L"." + std::to_wstring(version[3]);
    }
    return text;
}

ReleaseVersion CurrentVersion()
{
    return {ATG_VERSION_MAJOR, ATG_VERSION_MINOR, ATG_VERSION_PATCH, 0};
}
