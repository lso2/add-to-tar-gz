// sevenzip.cpp: path of the bundled 7za.exe and the exact 7-Zip switches for each stage.
// Globals: none beyond sevenzip.h.
// Doc: docs/notes/src/worker/sevenzip.md
#include "sevenzip.h"
#include "paths.h"

std::wstring SevenZipPath()
{
    // The 7za.exe shipped with AddToTarGz, the version every test ran against.
    return JoinPath(ModuleFolder(nullptr), L"7za.exe");
}

std::vector<std::wstring> TarCreateArgs(const std::wstring& itemPath)
{
    // "--" ends the switch list. No path after it is read as a switch.
    return {L"a", L"-ttar", L"-an", L"-so", L"-bsp0", L"-bso0", L"-sccUTF-8", L"--", itemPath};
}

std::vector<std::wstring> TarListArgs(const std::wstring& listPath)
{
    // 7-Zip reads "@" as a list file only before "--", and the list is the job's own file in the temp folder.
    return {L"a", L"-ttar", L"-an", L"-so", L"-bsp0", L"-bso0", L"-sccUTF-8", L"-scsUTF-8", L"@" + listPath};
}

std::vector<std::wstring> GzipCreateArgs(const std::wstring& nameInGzip, int level)
{
    // The menu DLL passes the CompressionLevel setting. Time and size of each level are measured in the doc.
    return {L"a", L"-tgzip", L"-an", L"-si" + nameInGzip, L"-so", L"-mx" + std::to_wstring(level),
            L"-bsp0", L"-bso0", L"-sccUTF-8"};
}

std::vector<std::wstring> GzipReadArgs()
{
    return {L"x", L"-tgzip", L"-si", L"-so", L"-bsp0", L"-bso0", L"-sccUTF-8"};
}

std::vector<std::wstring> TarExtractArgs(const std::wstring& outputFolder)
{
    // The output folder is new. With -aoa, a later entry of the same name overwrites an earlier one, as in tar.
    return {L"x", L"-ttar", L"-si", L"-aoa", L"-y", L"-o" + outputFolder, L"-bsp0", L"-bso0", L"-sccUTF-8"};
}
