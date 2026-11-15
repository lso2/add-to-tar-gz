// cmdline.h: builds command lines that CommandLineToArgvW and the MSVC runtime split back into the same arguments.
// Globals: AppendArg.
// Doc: docs/notes/src/shared/cmdline.md
#pragma once
#include <string>

// Appends a space (when cmd is not empty) and the argument, quoted only when it holds a space, tab or quote.
void AppendArg(std::wstring& cmd, const std::wstring& arg);
