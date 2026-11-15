// args.h: command line of AddToTarGz.exe.
// AddToTarGz.exe --add [--one] [--ext tar.gz|tgz] [--level 1-9] [--silent] [--] <path>...
// AddToTarGz.exe --extract [--here | --dest <folder> | --pick-dest] [--silent] [--] <path>...
// The menu DLL passes the paths of --one as --paths-memory <handle>: inherited shared memory with no size limit.
// Globals: Mode, Options, ParseArgs, UsageText.
// Doc: docs/notes/src/worker/args.md
#pragma once
#include <string>
#include <vector>
#include "names.h"

enum class Mode { None, Add, Extract };

struct Options {
    Mode mode = Mode::None;
    ArchiveExt ext = ArchiveExt::TarGz;
    int level = 5;                    // gzip level of --add
    bool one = false;                 // --one: every path goes into one archive named after the first path's folder
    bool here = false;                // --here: the archive's entries go straight into the archive's folder
    std::wstring dest;                // --dest: the folder that receives each "<name>\" folder
    bool pickDest = false;            // --pick-dest: dest is picked in a folder dialog before the jobs
    bool silent = false;
    std::vector<std::wstring> paths;  // full paths from GetFullPathNameW
};

// Fills options from this process's command line. Returns false with a message for bad input.
bool ParseArgs(Options& options, std::wstring& error);

// Usage text for the command line error report.
const wchar_t* UsageText();
