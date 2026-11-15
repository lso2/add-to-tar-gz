// module.cpp: module handle and live-object count of targz_context.dll.
// Globals: none beyond module.h.
// Doc: docs/notes/src/menu/module.md
#include "module.h"

namespace {

HMODULE g_module = nullptr;
LONG g_refCount = 0;

}  // namespace

void SetModule(HMODULE module)
{
    g_module = module;
}

HMODULE Module()
{
    return g_module;
}

void ModuleAddRef()
{
    InterlockedIncrement(&g_refCount);
}

void ModuleRelease()
{
    InterlockedDecrement(&g_refCount);
}

LONG ModuleRefCount()
{
    return InterlockedCompareExchange(&g_refCount, 0, 0);
}
