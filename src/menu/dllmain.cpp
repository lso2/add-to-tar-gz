// dllmain.cpp: entry points of targz_context.dll that Windows and COM call.
// Installer.iss writes the registration, and the DLL exports no DllRegisterServer.
// Globals: DllMain, DllGetClassObject, DllCanUnloadNow.
// Doc: docs/notes/src/menu/dllmain.md
#include <windows.h>
#include <new>
#include "class_factory.h"
#include "clsid.h"
#include "module.h"

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        SetModule(module);
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}

_Check_return_ STDAPI DllGetClassObject(_In_ REFCLSID clsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* object)
{
    if (object == nullptr) {
        return E_POINTER;
    }
    *object = nullptr;
    if (!IsAddToTarGzMenuClsid(clsid)) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    ClassFactory* factory = new (std::nothrow) ClassFactory();
    if (factory == nullptr) {
        return E_OUTOFMEMORY;
    }
    const HRESULT result = factory->QueryInterface(riid, object);
    factory->Release();
    return result;
}

__control_entrypoint(DllExport) STDAPI DllCanUnloadNow()
{
    return ModuleRefCount() == 0 ? S_OK : S_FALSE;
}
