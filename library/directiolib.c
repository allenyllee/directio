/*
 * Copyright (C) 2011 Jernej Simončič
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */

#define UNICODE
#define NOGDI
#include <windows.h>
#include <wchar.h>
#define BUILDING_DIRECTIOLIB
#include "directiolib.h"

typedef void (WINAPI *GETNATIVESYSTEMINFO)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *ISWOW64PROCESS)(HANDLE,PBOOL);

HINSTANCE hDirectIoLib; /* assigned hInstance of the DLL in DllMain */

/* GetModuleFileName can return \\?\ paths, so MAX_PATH might be too short */
#define DIRECTIO_MAX_PATH  32767

wchar_t DriverPath[DIRECTIO_MAX_PATH];
HANDLE hDirectIO = INVALID_HANDLE_VALUE;

#define DIRECTIO_SERVICE L"DirectIO"
#define DIRECTIO_FILE L"\\\\.\\DirectIO"

#define DIRECTIO_CPU_UNSUPPORTED 0
#define DIRECTIO_CPU_X86         1
#define DIRECTIO_CPU_AMD64       2

/* save DLL's HINSTANCE (needed to access resources) */
BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		hDirectIoLib = hInstance;
		break;
	}

	return 1;
}

int DirectIO_CPUArchitecture()
{
#if !defined(_M_AMD64) && !defined(_M_IX86)
	#error "Only x86 and AMD64 are supported"
#endif
#ifdef _WIN64

	return DIRECTIO_CPU_AMD64;

#else

	GETNATIVESYSTEMINFO fnGetNativeSystemInfo;
	ISWOW64PROCESS fnIsWow64Process;
	SYSTEM_INFO SysInfo;
	BOOL bIsWow64 = FALSE;

	int result = DIRECTIO_CPU_UNSUPPORTED;

	fnIsWow64Process = (ISWOW64PROCESS) GetProcAddress(GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

	if (NULL == fnIsWow64Process) {
		/* function doesn't exist - we're on x86 */
		result = DIRECTIO_CPU_X86;
	} else {
		if (fnIsWow64Process(GetCurrentProcess(),&bIsWow64)) {
			if (!bIsWow64) {
				/* we're not on Wow64, so this is x86 */
				result = DIRECTIO_CPU_X86;
			} else {
				/* WOW64 - check if this is AMD64 */
				fnGetNativeSystemInfo = (GETNATIVESYSTEMINFO) GetProcAddress(
				                          GetModuleHandle(TEXT("kernel32")),"GetNativeSystemInfo");

				if (NULL != fnGetNativeSystemInfo) {
					fnGetNativeSystemInfo(&SysInfo);
					if (SysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
						result = DIRECTIO_CPU_AMD64;
					}
				}
			}
		}
	}

	return result;

#endif
}

/* extracts DirectIO driver, sets the path to it */
BOOL DirectIO_PrepareDriver()
{
	HRSRC hResource;
	HGLOBAL hDriver;
	LPVOID lpDriver;
	DWORD dwSize;
	HANDLE hDriverFile;
	HANDLE hDriverMap;
	LPVOID lpMapView;
	BOOL success = 0;

	if (!GetTempPath( DIRECTIO_MAX_PATH, DriverPath )) {
		return 0;
	}
	#define DIRECTIO_DRIVER L"DirectIO.sys"
	wcsncat(DriverPath,DIRECTIO_DRIVER,DIRECTIO_MAX_PATH-wcslen(DIRECTIO_DRIVER)-1);

	/* find the driver resource for the correct platform */
	switch (DirectIO_CPUArchitecture()) {
#ifndef _WIN64
	case DIRECTIO_CPU_X86:
		hResource = FindResource( hDirectIoLib, MAKEINTRESOURCE(RC_DRV_X86), RT_RCDATA );
		break;
#endif

	case DIRECTIO_CPU_AMD64:
		hResource = FindResource( hDirectIoLib, MAKEINTRESOURCE(RC_DRV_AMD64), RT_RCDATA );
		break;

	default:
		return 0;

	}
	if (NULL == hResource)
		return 0;

	/* load the resource */
	hDriver = LoadResource( hDirectIoLib, hResource );
	if (NULL == hDriver)
		return 0;

	/* get the pointer to the resource and it's size */
	lpDriver = LockResource( hDriver );
	if (NULL == lpDriver)
		return 0;
	dwSize = SizeofResource( hDirectIoLib, hResource );
	if (0 == dwSize)
		return 0;

	/* open the file the driver will be saved to and map it */
	hDriverFile = CreateFile( DriverPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if (INVALID_HANDLE_VALUE == hDriverFile)
		return 0;
	hDriverMap = CreateFileMapping( hDriverFile, NULL, PAGE_READWRITE, 0, dwSize, NULL );
	if (NULL == hDriverMap)
		goto cleanup1;
	lpMapView = MapViewOfFile( hDriverMap, FILE_MAP_WRITE, 0, 0, 0 );
	if (NULL == lpMapView)
		goto cleanup2;

	/* write the driver */
	CopyMemory( lpMapView, lpDriver, dwSize );
	success = 1;

	UnmapViewOfFile( lpMapView );
cleanup2:
	CloseHandle( hDriverMap );
cleanup1:
	CloseHandle( hDriverFile );

	return success;
}

/* stop, uninstall and delete DirectIO driver */
void _stdcall DirectIO_Uninstall()
{
	SC_HANDLE hSCManager;
	SC_HANDLE hService;
	SERVICE_STATUS stat;

	hSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	if (hSCManager == NULL)
		return;

	hService = OpenService( hSCManager, DIRECTIO_SERVICE, SERVICE_ALL_ACCESS );
	if (hService == NULL) {
		CloseServiceHandle( hSCManager );
		return;
	}

	ControlService( hService, SERVICE_CONTROL_STOP, &stat );

	DeleteService( hService );

	CloseServiceHandle( hService );
	CloseServiceHandle( hSCManager );

	DeleteFile( DriverPath );

	return;
}

/* install and start the DirectIO driver */
BOOL DirectIO_Install()
{
	SC_HANDLE hSCManager;
	SC_HANDLE hService;	
	BOOL result;

	DirectIO_Uninstall();

	hSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	if (hSCManager == NULL)		
		return 0;

	hService = CreateService( hSCManager, DIRECTIO_SERVICE, L"DirectIO driver", SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER, SERVICE_SYSTEM_START, SERVICE_ERROR_NORMAL, DriverPath, NULL, NULL,
		NULL, NULL, NULL );

	CloseServiceHandle( hSCManager );

	if (hService == NULL)
		return 0;

	if (!StartService(hService, 0, NULL)) {		
		if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) {
			result = 1; /* service is running, so we're ok */
		} else {
			result = 0;
		}
	} else {
		result = 1;
	}
		
	CloseServiceHandle( hService );

	return result;
}

/* DirectIO initialization - load driver (if needed), connect to it */
DIRECTIOLIB_EXTERN BOOL _stdcall DirectIO_Init()
{
	/* try to open existing driver, if available */
	hDirectIO = CreateFile(DIRECTIO_FILE, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hDirectIO == INVALID_HANDLE_VALUE)
	{
		if (!DirectIO_PrepareDriver()) {
			return 0;
		}

		if (!DirectIO_Install()) {
			DeleteFile( DriverPath );
			return 0;
		}

		hDirectIO = CreateFile(DIRECTIO_FILE, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
		                       NULL);

		if (hDirectIO == INVALID_HANDLE_VALUE) {
			DirectIO_Uninstall();
			return 0;
		}
	}

	return 1;

}

/* DirectIO deinitialization - close driver handle, unload it
 * always call this when done working with DirectIO, so that the driver doesn't stay loaded
 */
DIRECTIOLIB_EXTERN void _stdcall DirectIO_DeInit()
{
	if (hDirectIO != INVALID_HANDLE_VALUE) {
		CloseHandle(hDirectIO);
		hDirectIO = INVALID_HANDLE_VALUE;
	}

	DirectIO_Uninstall();
}

/* write to port
 *  PortVal: value to write to port
 *  Port: port to write to
 *  Size: size of port write (psByte, psWord, psDWord)
 */
DIRECTIOLIB_EXTERN BOOL _stdcall DirectIO_WritePort(ULONG PortVal, USHORT Port, DIRECTIO_PORT_SIZE Size)
{
	DIRECTIO_PORT directio_port;
	DWORD junk;

	if (hDirectIO == INVALID_HANDLE_VALUE) {
		return 0;
	}

	directio_port.Size = Size;
	directio_port.PortVal = PortVal;
	directio_port.Port = Port;

	return DeviceIoControl(hDirectIO, IOCTL_DIRECTIO_WRITE, &directio_port, sizeof(directio_port), NULL, 0, &junk, NULL);

}

/* read port
 *  PortVal: value to read from port
 *  Port: port to read from
 *  Size: size of port read (psByte, psWord, psDWord)
 */
DIRECTIOLIB_EXTERN BOOL _stdcall DirectIO_ReadPort(PULONG PortVal, USHORT Port, DIRECTIO_PORT_SIZE Size)
{
	DIRECTIO_PORT directio_port;
	DWORD junk;

	if (hDirectIO == INVALID_HANDLE_VALUE) {
		return 0;
	}

	directio_port.Size = Size;
	directio_port.Port = Port;

	return DeviceIoControl(hDirectIO, IOCTL_DIRECTIO_READ, &directio_port, sizeof(directio_port), PortVal, sizeof(PortVal), &junk,
	                       NULL);

}

