// clsid.h: class identifiers of the context menu handler in targz_context.dll.
// Installer.iss registers one per build, {BF8A488E-EBA4-4D70-8486-<build time>}, under HKCU\Software\Classes\CLSID.
// Globals: IsAddToTarGzMenuClsid.
// Doc: docs/notes/src/menu/clsid.md
#pragma once
#include <guiddef.h>

// True for every {BF8A488E-EBA4-4D70-8486-xxxxxxxxxxxx}. Each build of Installer.iss registers a new one of these
// for its own DLL, and a running Explorer that still has an earlier build loaded creates the next menu from it.
inline bool IsAddToTarGzMenuClsid(const CLSID& clsid)
{
    return clsid.Data1 == 0xbf8a488e && clsid.Data2 == 0xeba4 && clsid.Data3 == 0x4d70 && clsid.Data4[0] == 0x84 &&
           clsid.Data4[1] == 0x86;
}
