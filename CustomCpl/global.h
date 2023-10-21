#pragma once
#include "guid.h"
#include "resource.h"
extern HINSTANCE g_hInst;
extern WCHAR g_szExtTitle[512];

extern void DllAddRef();
extern void DllRelease();