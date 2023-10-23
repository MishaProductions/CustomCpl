#include "pch.h"
#include "global.h"
#include "CplProvider.h"
#include "hostfxr.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "nethost.h"

// Header files copied from https://github.com/dotnet/core-setup
#include "coreclr_delegates.h"
#include "hostfxr.h"

#define STR(s) L ## s
#define CH(c) L ## c
#define DIR_SEPARATOR L'\\'

#define string_compare wcscmp
using string_t = std::basic_string<char_t>;

hostfxr_initialize_for_dotnet_command_line_fn init_for_cmd_line_fptr;
hostfxr_initialize_for_runtime_config_fn init_for_config_fptr;
hostfxr_get_runtime_delegate_fn get_delegate_fptr;
hostfxr_run_app_fn run_app_fptr;
hostfxr_close_fn close_fptr;

// must match in CustomCPLImpl.Creator
typedef void* (CORECLR_DELEGATE_CALLTYPE* createpage_ptr)();

#define NOT_IMPLEMENTED MessageBox(NULL, TEXT(__FUNCTION__), TEXT("Non implementented function in CElementProvider"), MB_ICONERROR)
#define SHOW_ERROR(x) MessageBox(NULL, TEXT(x), TEXT("Error in CElementProvider"), MB_ICONERROR)


CplProvider::CplProvider() : _punkSite(NULL)
{
	if (FAILED(InitProcessPriv(14, NULL, 1, true)))
	{
		SHOW_ERROR("Failed to initialize DirectUI\n");
	}
	if (FAILED(InitThread(2)))
	{
		SHOW_ERROR("Failed to initialize DirectUI for thread\n");
	}
	refCount = 1;
	DllAddRef();
}

CplProvider::~CplProvider()
{

	UnInitThread();
	UnInitProcessPriv((unsigned short*)g_hInst);

	DllRelease();
}


HRESULT CplProvider::QueryInterface(REFIID riid, __out void** ppv)
{
	static const QITAB qit[] = {
		QITABENT(CplProvider, IDUIElementProviderInit),
		QITABENT(CplProvider, IFrameNotificationClient),
		QITABENT(CplProvider, IFrameShellViewClient),
		QITABENT(CplProvider, IObjectWithSite),
		QITABENT(CplProvider, IServiceProvider),
		{ 0 },
	};
	HRESULT hr = QISearch(this, qit, riid, ppv);

	if (hr != S_OK)
	{
		hr = DirectUI::XProvider::QueryInterface(riid, ppv);
	}
	if (hr != S_OK)
	{
		WCHAR szGuid[40] = { 0 };

		swprintf_s(szGuid, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);

		//MessageBox(NULL, szGuid, TEXT("Unknown interface in CplProvider::QueryInterface()"), MB_ICONERROR);
	}
	return hr;
}

ULONG CplProvider::AddRef()
{
	return DirectUI::XProvider::AddRef();
}

ULONG CplProvider::Release()
{
	ULONG cRef = InterlockedDecrement(&refCount);
	if (0 == cRef)
	{
		delete this;
	}
	return cRef;
}
void* load_library(const char_t* path)
{
	HMODULE h = ::LoadLibraryW(path);
	assert(h != nullptr);
	return (void*)h;
}
void* get_export(void* h, const char* name)
{
	void* f = ::GetProcAddress((HMODULE)h, name);
	assert(f != nullptr);
	return f;
}
// <SnippetLoadHostFxr>
  // Using the nethost library, discover the location of hostfxr and get exports
bool load_hostfxr(const char_t* assembly_path)
{
	get_hostfxr_parameters params{ sizeof(get_hostfxr_parameters), assembly_path, nullptr };
	// Pre-allocate a large buffer for the path to hostfxr
	char_t buffer[MAX_PATH];
	size_t buffer_size = sizeof(buffer) / sizeof(char_t);
	int rc = get_hostfxr_path(buffer, &buffer_size, &params);
	if (rc != 0)
		return false;

	// Load hostfxr and get desired exports
	void* lib = load_library(buffer);
	init_for_cmd_line_fptr = (hostfxr_initialize_for_dotnet_command_line_fn)get_export(lib, "hostfxr_initialize_for_dotnet_command_line");
	init_for_config_fptr = (hostfxr_initialize_for_runtime_config_fn)get_export(lib, "hostfxr_initialize_for_runtime_config");
	get_delegate_fptr = (hostfxr_get_runtime_delegate_fn)get_export(lib, "hostfxr_get_runtime_delegate");
	run_app_fptr = (hostfxr_run_app_fn)get_export(lib, "hostfxr_run_app");
	close_fptr = (hostfxr_close_fn)get_export(lib, "hostfxr_close");

	return (init_for_config_fptr && get_delegate_fptr && close_fptr);
}
// </SnippetLoadHostFxr>
 // <SnippetInitialize>
	// Load and initialize .NET Core and get desired function pointer for scenario
load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(const char_t* config_path)
{
	// Load .NET Core
	void* load_assembly_and_get_function_pointer = nullptr;
	hostfxr_handle cxt = nullptr;
	int rc = init_for_config_fptr(config_path, nullptr, &cxt);
	if (rc != 0 && cxt == nullptr)
	{
		std::cerr << "Init failed: " << std::hex << std::showbase << rc << std::endl;
		close_fptr(cxt);
		return nullptr;
	}

	// Get the load assembly function pointer
	rc = get_delegate_fptr(
		cxt,
		hdt_load_assembly_and_get_function_pointer,
		&load_assembly_and_get_function_pointer);
	if (rc != 0 || load_assembly_and_get_function_pointer == nullptr)
		std::cerr << "Get delegate failed: " << std::hex << std::showbase << rc << std::endl;

	close_fptr(cxt);
	return (load_assembly_and_get_function_pointer_fn)load_assembly_and_get_function_pointer;
}
// </SnippetInitialize>

LRESULT CALLBACK MainWndProc(
	HWND hwnd,        // handle to window
	UINT uMsg,        // message identifier
	WPARAM wParam,    // first message parameter
	LPARAM lParam)    // second message parameter
{
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HRESULT CplProvider::CreateDUI(DirectUI::IXElementCP* a, HWND* result_handle)
{	//
	XProvider::CreateDUI(a, result_handle); //call to avoid crashes upon exit
	   // STEP 1: Load HostFxr and get exported hosting functions
	   //
	if (!load_hostfxr(nullptr))
	{
		assert(false && "Failure: load_hostfxr()");
		return EXIT_FAILURE;
	}


	// Get root path of our dll
	string_t root_path;
	wchar_t pBuf[256];
	size_t len = sizeof(pBuf);
	int bytes = GetModuleFileNameW(g_hInst, pBuf, 256);
	string_t root_file = pBuf;

	const size_t last_slash_idx = root_file.rfind('\\');
	if (std::string::npos != last_slash_idx)
	{
		root_path = root_file.substr(0, last_slash_idx);
	}
	root_path = root_path + L"\\";
	//
	// STEP 2: Initialize and start the .NET Core runtime
	//
	const string_t config_path = root_path + STR("CustomCPLImpl.runtimeconfig.json");
	load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = nullptr;
	load_assembly_and_get_function_pointer = get_dotnet_load_assembly(config_path.c_str());
	if (load_assembly_and_get_function_pointer == nullptr)
	{
		return S_OK;
	}

	//
   // STEP 3: Load managed assembly and get function pointer to a managed method
   //
	const string_t dotnetlib_path = root_path + STR("CustomCPLImpl.dll");
	const char_t* dotnet_type = STR("CustomCPLImpl.Creator, CustomCPLImpl");
	const char_t* dotnet_type_method = STR("CreatePage");
	// <SnippetLoadAndGet>
	// Function pointer to managed delegate
	createpage_ptr hello = nullptr;
	int rc = load_assembly_and_get_function_pointer(
		dotnetlib_path.c_str(),
		dotnet_type,
		dotnet_type_method,
		STR("CustomCPLImpl.Creator+CreatePageDelegate, CustomCPLImpl") /*delegate_type_name*/,
		nullptr,
		(void**)&hello);
	// </SnippetLoadAndGet>

	if (rc != 0 || hello == nullptr)
	{
		return S_OK;
	}

	HWND usercontrolHwnd = (HWND)hello();

	WNDCLASSEXW classinfo = {0};
	classinfo.cbSize = sizeof(WNDCLASSEXW);
	classinfo.hInstance = g_hInst;
	classinfo.lpszClassName = L"XBabyHost2";
	classinfo.hbrBackground = (HBRUSH)GetStockObject(COLOR_ACTIVECAPTION);
	classinfo.lpfnWndProc = MainWndProc;

	ATOM classs = RegisterClassExW(&classinfo);

	if (classs == NULL)
	{
		// error
		HRESULT hr = GetLastError();
		if (hr != ERROR_CLASS_ALREADY_EXISTS)
			return S_OK;
	}

	HWND sink = a->GetNotificationSinkHWND();
	HWND registerr = CreateWindowExW(0, L"XBabyHost2", 0, 0x46000000 | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, sink, 0, g_hInst, 0);
	HWND hr3 = SetParent(usercontrolHwnd, registerr);

	// update size of our usercontrol
	RECT hostRect = {0};
	GetWindowRect(registerr, &hostRect);

	// todo: properly set size
	BOOL x = SetWindowPos(usercontrolHwnd, NULL, 0, 0, 20000, 20000, 0);
	*result_handle = registerr;
	return 0;
}
#pragma warning( push )
#pragma warning( disable : 4312 ) // disable warning about compiler complaining about casting ID to pointer
HRESULT STDMETHODCALLTYPE CplProvider::SetResourceID(UINT id)
{
	WCHAR buffer[264];
	StringCchCopyW(buffer, 260, L"main");

	//First parmeter: hinstance of module
	//2nd: Resource ID of uifile
	//3rd param: The main resid value
	int hr = DirectUI::XResourceProvider::Create(g_hInst, (UCString)id, (UCString)buffer, 0, &this->resourceProvider);
	if (SUCCEEDED(hr))
	{
		hr = DirectUI::XProvider::Initialize(NULL, (IXProviderCP*)this->resourceProvider);
		if (!SUCCEEDED(hr))
		{
			WCHAR szResource[40] = { 0 };
			swprintf(szResource, 40, L"%d", id);

			MessageBox(NULL, szResource, TEXT("CElementProvider::SetResourceId Failed to initialize xprovider"), MB_ICONERROR);
		}
	}
	else
	{
		WCHAR szResource[40] = { 0 };
		swprintf(szResource, 40, L"%d", id);

		MessageBox(NULL, szResource, TEXT("CElementProvider::SetResourceId failed to create xprovider"), MB_ICONERROR);
	}
	return hr;
}
#pragma warning( pop )


HRESULT STDMETHODCALLTYPE CplProvider::OptionallyTakeInitialFocus(BOOL* result)
{
	*result = 0;
	return 0;
}

HRESULT STDMETHODCALLTYPE CplProvider::LayoutInitialized()
{
	return S_OK;
}
HRESULT STDMETHODCALLTYPE CplProvider::Notify(WORD* param)
{
	//the param is text

	if (!StrCmpCW((LPCWSTR)param, L"SettingsChanged"))
	{
		//This is invoked when the UI is refreshed!
	}

	if (!StrCmpCW((LPCWSTR)param, L"SearchText"))
	{
		//Sent when search text modified/added
		WCHAR value[264] = { 0 };
		WCHAR path[48];
		GUID IID_IFrameManager = {};
		GUID SID_STopLevelBrowser = {};
		HRESULT hr = CLSIDFromString(L"{4c96be40-915c-11cf-99d3-00aa004ae837}", (LPCLSID)&SID_STopLevelBrowser);
		if (SUCCEEDED(hr))
		{
			hr = CLSIDFromString(L"{31e4fa78-02b4-419f-9430-7b7585237c77}", (LPCLSID)&IID_IFrameManager);
			if (SUCCEEDED(hr))
			{
				IPropertyBag* bag = NULL;
				HRESULT hr = IUnknown_QueryService(_punkSite, IID_IFrameManager, IID_IPropertyBag, (LPVOID*)&bag);
				if (SUCCEEDED(hr))
				{
					if (SUCCEEDED(PSPropertyBag_ReadStr(bag, L"SearchText", value, 260)) && value[0])
					{
						if (SUCCEEDED(StringCchPrintfW(path, 41, L"::%s", L"{26ee0668-a00a-44d7-9371-beb064c98683}")))
						{
							LPITEMIDLIST pidlist;
							if (SUCCEEDED(SHParseDisplayName(path, NULL, &pidlist, 0, NULL)))
							{
								IShellBrowser* browser = NULL;
								if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IShellBrowser, (LPVOID*)&browser)))
								{
									hr = browser->BrowseObject(pidlist, SBSP_ACTIVATE_NOFOCUS | SBSP_SAMEBROWSER | SBSP_CREATENOHISTORY);
									browser->Release();
								}
							}
							ILFree(pidlist);
						}
					}
				}
			}
		}
	}
	return 0;
}
HRESULT STDMETHODCALLTYPE CplProvider::OnNavigateAway() {
	//TODO: this causes a crash
	//DirectUI::XProvider::SetHandleEnterKey(false);
	//SetDefaultButtonTracking(false);
	return 0;
}
HRESULT STDMETHODCALLTYPE CplProvider::OnInnerElementDestroyed() {
	// TODO: unload the CLR
	return 0;
}

HRESULT STDMETHODCALLTYPE CplProvider::OnSelectedItemChanged()
{
	NOT_IMPLEMENTED;
	return 0;
}
HRESULT STDMETHODCALLTYPE CplProvider::OnSelectionChanged()
{
	//NOT_IMPLEMENTED;
	return 0;
}
HRESULT STDMETHODCALLTYPE CplProvider::OnContentsChanged()
{
	//NOT_IMPLEMENTED;
	return 0;
}
HRESULT STDMETHODCALLTYPE CplProvider::OnFolderChanged()
{
	//NOT_IMPLEMENTED;
	return 0;
}

HRESULT STDMETHODCALLTYPE CplProvider::QueryService(
	REFGUID guidService,
	REFIID riid,
	void** ppvObject)
{
	*ppvObject = 0;
	NOT_IMPLEMENTED;
	return E_NOTIMPL;
}

HRESULT CplProvider::SetSite(IUnknown* punkSite)
{
	if (punkSite != NULL)
	{
		IUnknown_Set((IUnknown**)&this->Site, punkSite);
		IUnknown_Set((IUnknown**)&this->_punkSite, punkSite);
	}

	return S_OK;
}

HRESULT CplProvider::GetSite(REFIID riid, void** ppvSite)
{
	NOT_IMPLEMENTED;

	if (Site == NULL)
	{
		return E_FAIL;
	}

	return Site->QueryInterface(riid, ppvSite);
}
