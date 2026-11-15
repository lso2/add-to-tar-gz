// module.h: the DLL's module handle and the count of live COM objects and server locks.
// DllCanUnloadNow answers from this count. COM never unloads the DLL while an object is alive.
// Globals: SetModule, Module, ModuleAddRef, ModuleRelease, ModuleRefCount.
// Doc: docs/notes/src/menu/module.md
#pragma once
#include <windows.h>

void SetModule(HMODULE module);
HMODULE Module();
void ModuleAddRef();
void ModuleRelease();
LONG ModuleRefCount();
