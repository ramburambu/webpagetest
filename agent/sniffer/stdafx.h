// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#define PSAPI_VERSION 1
#include "targetver.h"

#define INCL_WINSOCK_API_TYPEDEFS 1

#define WIN32_LEAN_AND_MEAN
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <Psapi.h>
#include <tchar.h>
#undef _WIN32_WINNT
#define _WIN32_WINNT  0x0600
#include <Ws2tcpip.h>
#undef _WIN32_WINNT
#define _WIN32_WINNT  0x0501
#include <Wincrypt.h>

#include <shlobj.h>
#include <atlstr.h>
#include <atlcoll.h>
