// sevenzip.h: path of the bundled 7za.exe and the argument list of each 7-Zip stage.
// Every stage reads standard input or writes standard output. No intermediate .tar file exists.
// Globals: SevenZipPath, TarCreateArgs, TarListArgs, GzipCreateArgs, GzipReadArgs, TarExtractArgs.
// Doc: docs/notes/src/worker/sevenzip.md
#pragma once
#include <string>
#include <vector>

// Full path of 7za.exe in the folder of AddToTarGz.exe. The build and the installer place it there.
std::wstring SevenZipPath();

// Tar stream of one item on standard output. 7-Zip stores the entries of an absolute item path relative
// to the item's parent folder. The archive's root entry is then the item's own name.
std::vector<std::wstring> TarCreateArgs(const std::wstring& itemPath);

// Tar stream on standard output of every item in a UTF-8 7-Zip list file. 7-Zip stores the entries of each
// absolute path relative to that path's parent folder. Every listed item is then a root entry of the archive.
std::vector<std::wstring> TarListArgs(const std::wstring& listPath);

// Gzip of standard input on standard output at gzip level 1 to 9. nameInGzip is the file name stored in the
// gzip header.
std::vector<std::wstring> GzipCreateArgs(const std::wstring& nameInGzip, int level);

// Gunzip of standard input on standard output.
std::vector<std::wstring> GzipReadArgs();

// Extraction of a tar stream on standard input into outputFolder.
std::vector<std::wstring> TarExtractArgs(const std::wstring& outputFolder);
