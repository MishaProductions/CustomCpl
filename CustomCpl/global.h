#pragma once
#include "guid.h"
#include "resource.h"

//DirectUI
#include "..\dui70\DirectUI\DirectUI.h"
using namespace DirectUI;

extern HINSTANCE g_hInst;
extern WCHAR g_szExtTitle[512];

extern void DllAddRef();
extern void DllRelease();