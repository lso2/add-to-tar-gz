// cmdline.cpp: command line quoting that round-trips through CommandLineToArgvW.
// Globals: none beyond cmdline.h.
// Doc: docs/notes/src/shared/cmdline.md
#include "cmdline.h"

void AppendArg(std::wstring& cmd, const std::wstring& arg)
{
    if (!cmd.empty()) {
        cmd.push_back(L' ');
    }
    if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
        cmd += arg;
        return;
    }
    cmd.push_back(L'"');
    for (size_t i = 0;; ++i) {
        size_t backslashes = 0;
        while (i < arg.size() && arg[i] == L'\\') {
            ++i;
            ++backslashes;
        }
        if (i == arg.size()) {
            // Backslashes before the closing quote are doubled. The quote is then read as the delimiter.
            cmd.append(backslashes * 2, L'\\');
            break;
        }
        if (arg[i] == L'"') {
            cmd.append(backslashes * 2 + 1, L'\\');
        } else {
            cmd.append(backslashes, L'\\');
        }
        cmd.push_back(arg[i]);
    }
    cmd.push_back(L'"');
}
