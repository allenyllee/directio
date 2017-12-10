/* needed when compiling on mingw */
#ifndef CTL_CODE
	#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
	    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
	)
#endif
#ifndef METHOD_BUFFERED
	#define METHOD_BUFFERED                 0
#endif
#ifndef FILE_ANY_ACCESS
	#define FILE_ANY_ACCESS                 0
#endif

#define FILE_DEVICE_DIRECTIO 0x8D10
#define DIRECTIO_FUNCTION 0xD10

#define	IOCTL_DIRECTIO_READ 	CTL_CODE( FILE_DEVICE_DIRECTIO, DIRECTIO_FUNCTION + 0, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define	IOCTL_DIRECTIO_WRITE	CTL_CODE( FILE_DEVICE_DIRECTIO, DIRECTIO_FUNCTION + 1, METHOD_BUFFERED, FILE_ANY_ACCESS )

typedef enum {
	psByte  = 1,
	psWord  = 2,
	psDWord = 3
} DIRECTIO_PORT_SIZE;

typedef struct _DIRECTIO_PORT {
	USHORT Port;
	ULONG PortVal;
	DIRECTIO_PORT_SIZE Size;
} DIRECTIO_PORT, *PDIRECTIO_PORT;
