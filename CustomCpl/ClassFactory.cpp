#include "pch.h"
#include "global.h"
#include "ClassFactory.h"
#include "ElementProvider.h"


ClassFactory::ClassFactory(REFCLSID rclsid) : m_cRef(1), m_rclsid(rclsid)
{
    DllAddRef();
}

ClassFactory::~ClassFactory()
{
    DllRelease();
}

HRESULT ClassFactory::QueryInterface(__in REFIID riid, __deref_out void** ppv)
{
    static const QITAB qit[] = {
       QITABENT(ClassFactory, IClassFactory),
       { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

DWORD ClassFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

DWORD ClassFactory::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
    {
        delete this;
    }
    return cRef;
}

HRESULT ElementProvider_CreateInstance(__in REFIID riid, __deref_out void** ppv)
{
    HRESULT hr = S_ALLTHRESHOLD;
    ElementProvider* pElementProvider = new ElementProvider();
    hr = pElementProvider ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        hr = pElementProvider->QueryInterface(riid, ppv);
        pElementProvider->Release();
    }
    return hr;
}

HRESULT ClassFactory::CreateInstance(__in_opt IUnknown* punkOuter,
    __in REFIID riid,
    __deref_out void** ppv)
{
    *ppv = NULL;

    HRESULT hr = !punkOuter ? S_OK : CLASS_E_NOAGGREGATION;
    if (SUCCEEDED(hr))
    {
        if (CLSID_FolderViewImplElement == m_rclsid)
        {
            hr = ElementProvider_CreateInstance(riid, ppv);
        }
        else
        {
            hr = E_NOINTERFACE;

            WCHAR szGuid[40] = { 0 };

            swprintf(szGuid, 40, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);

            MessageBox(NULL, szGuid, TEXT("Unknown interface in CFolderViewImplClassFactory::CreateInstance()"), 0);
        }
    }
    return hr;
}

HRESULT ClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
    {
        DllAddRef();
    }
    else
    {
        DllRelease();
    }
    return S_OK;
}
