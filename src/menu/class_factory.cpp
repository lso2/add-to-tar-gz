// class_factory.cpp: COM class factory for the ContextMenu object.
// Globals: none beyond class_factory.h.
// Doc: docs/notes/src/menu/class_factory.md
#include "class_factory.h"
#include "context_menu.h"
#include "module.h"
#include <new>

ClassFactory::ClassFactory()
{
    ModuleAddRef();
}

ClassFactory::~ClassFactory()
{
    ModuleRelease();
}

IFACEMETHODIMP ClassFactory::QueryInterface(REFIID riid, void** object)
{
    if (object == nullptr) {
        return E_POINTER;
    }
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
        *object = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }
    *object = nullptr;
    return E_NOINTERFACE;
}

IFACEMETHODIMP_(ULONG) ClassFactory::AddRef()
{
    return static_cast<ULONG>(InterlockedIncrement(&refs_));
}

IFACEMETHODIMP_(ULONG) ClassFactory::Release()
{
    const LONG count = InterlockedDecrement(&refs_);
    if (count == 0) {
        delete this;
    }
    return static_cast<ULONG>(count);
}

IFACEMETHODIMP ClassFactory::CreateInstance(IUnknown* outer, REFIID riid, void** object)
{
    if (object == nullptr) {
        return E_POINTER;
    }
    *object = nullptr;
    if (outer != nullptr) {
        return CLASS_E_NOAGGREGATION;
    }
    ContextMenu* menu = new (std::nothrow) ContextMenu();
    if (menu == nullptr) {
        return E_OUTOFMEMORY;
    }
    const HRESULT result = menu->QueryInterface(riid, object);
    menu->Release();
    return result;
}

IFACEMETHODIMP ClassFactory::LockServer(BOOL lock)
{
    if (lock) {
        ModuleAddRef();
    } else {
        ModuleRelease();
    }
    return S_OK;
}
