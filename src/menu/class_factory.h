// class_factory.h: COM class factory of targz_context.dll, which creates the ContextMenu object.
// Globals: ClassFactory.
// Doc: docs/notes/src/menu/class_factory.md
#pragma once
#include <unknwn.h>

class ClassFactory final : public IClassFactory {
public:
    ClassFactory();
    ~ClassFactory();

    IFACEMETHODIMP QueryInterface(REFIID riid, void** object) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

    IFACEMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** object) override;
    IFACEMETHODIMP LockServer(BOOL lock) override;

private:
    LONG refs_ = 1;
};
