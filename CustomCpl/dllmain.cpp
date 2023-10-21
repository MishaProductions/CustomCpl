// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "global.h"
#include "ClassFactory.h"

HINSTANCE g_hInst;
LONG g_cRefModule = 0;
WCHAR g_szExtTitle[512];

void DllAddRef()
{
    InterlockedIncrement(&g_cRefModule);
}

void DllRelease()
{
    InterlockedDecrement(&g_cRefModule);
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hModule;
        wcscpy_s(g_szExtTitle, 512, L"Custom CPL");
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

STDAPI DllCanUnloadNow(void)
{
    return g_cRefModule ? S_FALSE : S_OK;
}

STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv)
{
    *ppv = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    ClassFactory* pClassFactory = new ClassFactory(rclsid);
    if (pClassFactory)
    {
        hr = pClassFactory->QueryInterface(riid, ppv);
        pClassFactory->Release();
    }
    if (FAILED(hr))
    {
        WCHAR szGuid[100] = { 0 };

        swprintf(szGuid, 100, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            rclsid.Data1, rclsid.Data2, rclsid.Data3, rclsid.Data4[0], rclsid.Data4[1], rclsid.Data4[2], rclsid.Data4[3], rclsid.Data4[4], rclsid.Data4[5], rclsid.Data4[6], rclsid.Data4[7],
            riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);

        MessageBox(NULL, szGuid, TEXT("Unknown interface in DllGetClassObject()"), 0);
    }
    return hr;
    return S_FALSE;
}


