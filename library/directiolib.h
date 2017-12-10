#include "../driver/directio.h"
#include "directiorc.h"

#if defined(BUILDING_DIRECTIOLIB)
#define DIRECTIOLIB_EXTERN  __declspec(dllexport)
#else
#define DIRECTIOLIB_EXTERN  __declspec(dllimport)
#endif

DIRECTIOLIB_EXTERN BOOL WINAPI DirectIO_Init();
DIRECTIOLIB_EXTERN void WINAPI DirectIO_DeInit();
DIRECTIOLIB_EXTERN BOOL WINAPI DirectIO_WritePort(ULONG PortVal, USHORT Port, DIRECTIO_PORT_SIZE Size);
DIRECTIOLIB_EXTERN BOOL WINAPI DirectIO_ReadPort(PULONG PortVal, USHORT Port, DIRECTIO_PORT_SIZE Size);
