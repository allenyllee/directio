/*
 * DirectIO
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

#include <ntddk.h>
#include "directio.h"

#define DIRECTIO_DEVNAME L"\\Device\\DirectIO"
#define DIRECTIO_LNKNAME L"\\DosDevices\\DirectIO"

DRIVER_UNLOAD DirectIO_Unload;
DRIVER_DISPATCH DirectIO_DispatchCreateClose;
DRIVER_DISPATCH DirectIO_DispatchDeviceControl;

void DirectIO_Unload( IN PDRIVER_OBJECT DriverObject )
{
	UNICODE_STRING SymbolicLinkName;	
	NTSTATUS status;

	RtlInitUnicodeString( &SymbolicLinkName, DIRECTIO_LNKNAME );
	status = IoDeleteSymbolicLink( &SymbolicLinkName );

	if (NT_SUCCESS(status)) {
		IoDeleteDevice( DriverObject->DeviceObject );
	} else {
		KdPrint(( "DirectIO: error on IoDeleteSymbolicLink" ));
	}
}

NTSTATUS DirectIO_DispatchCreateClose( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
	UNREFERENCED_PARAMETER( DeviceObject );

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return STATUS_SUCCESS;
}

NTSTATUS DirectIO_DispatchDeviceControl( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
	PIO_STACK_LOCATION io_stack;
    NTSTATUS status;
	PDIRECTIO_PORT directio_port;
	PVOID PortVal;

	UNREFERENCED_PARAMETER( DeviceObject );

	/* defaults */
	Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
	Irp->IoStatus.Information = 0;

	io_stack = IoGetCurrentIrpStackLocation( Irp );

	if (io_stack && (io_stack->MajorFunction == IRP_MJ_DEVICE_CONTROL)) {

		if ((io_stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_DIRECTIO_READ)
		    || (io_stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_DIRECTIO_WRITE)) {

			/* parameter validation */
			if (io_stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DIRECTIO_PORT)) {
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
				goto bailout;
			}

			directio_port = (PDIRECTIO_PORT) Irp->AssociatedIrp.SystemBuffer;
			PortVal = Irp->AssociatedIrp.SystemBuffer;

			switch (directio_port->Size) {
			case psByte:
				if (io_stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_DIRECTIO_READ)
					directio_port->PortVal = (ULONG)READ_PORT_UCHAR( (PUCHAR)directio_port->Port );
				else
					WRITE_PORT_UCHAR( (PUCHAR)directio_port->Port, (UCHAR)directio_port->PortVal );

				Irp->IoStatus.Status = STATUS_SUCCESS;

				break;

			case psWord:
				if (io_stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_DIRECTIO_READ)
					directio_port->PortVal = (ULONG)READ_PORT_USHORT( (PUSHORT)directio_port->Port );
				else
					WRITE_PORT_USHORT( (PUSHORT)directio_port->Port, (USHORT)directio_port->PortVal );

				Irp->IoStatus.Status = STATUS_SUCCESS;

				break;

			case psDWord:
				if (io_stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_DIRECTIO_READ)
					directio_port->PortVal = READ_PORT_ULONG( (PULONG)directio_port->Port );
				else
					WRITE_PORT_ULONG( (PULONG)directio_port->Port, directio_port->PortVal );

				Irp->IoStatus.Status = STATUS_SUCCESS;

				break;
			}

			if (io_stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_DIRECTIO_READ) {
				if (io_stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(directio_port->PortVal)) {
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
				} else {
					Irp->IoStatus.Information = sizeof(directio_port->PortVal);
					RtlCopyBytes (PortVal, &directio_port->PortVal, sizeof(directio_port->PortVal));
				}
			}
		}

	}

bailout:
	status = Irp->IoStatus.Status;

	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return status;
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath )
{
	NTSTATUS status;
	PDEVICE_OBJECT DeviceObject;
	UNICODE_STRING DeviceName;
	UNICODE_STRING LinkName;

	UNREFERENCED_PARAMETER( RegistryPath );

	DriverObject->DriverUnload = DirectIO_Unload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DirectIO_DispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DirectIO_DispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DirectIO_DispatchDeviceControl;

	RtlInitUnicodeString( &DeviceName, DIRECTIO_DEVNAME );
	RtlInitUnicodeString( &LinkName, DIRECTIO_LNKNAME );

	status = IoCreateDevice( DriverObject, 0, &DeviceName, FILE_DEVICE_DIRECTIO, 0, FALSE, &DeviceObject );
	if (!NT_SUCCESS( status ))
		return status;

	status = IoCreateSymbolicLink( &LinkName, &DeviceName );
	if (!NT_SUCCESS( status ))
		return status;

	return STATUS_SUCCESS;
}
