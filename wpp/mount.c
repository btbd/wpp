#include "stdafx.h"

NTSTATUS MountPointsIoc(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	if (context) {
		IOC_REQUEST request = *(PIOC_REQUEST)context;
		ExFreePool(context);

		if (request.BufferLength >= sizeof(MOUNTMGR_MOUNT_POINTS)) {
			PMOUNTMGR_MOUNT_POINTS points = (PMOUNTMGR_MOUNT_POINTS)request.Buffer;
			for (DWORD i = 0; i < points->NumberOfMountPoints; ++i) {
				PMOUNTMGR_MOUNT_POINT point = &points->MountPoints[i];
				if (point->UniqueIdOffset) {
					point->UniqueIdLength = 0;
				}

				if (point->SymbolicLinkNameOffset) {
					point->SymbolicLinkNameLength = 0;
				}
			}
		}

		if (request.OldRoutine && irp->StackCount > 1) {
			return request.OldRoutine(device, irp, request.OldContext);
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS MountUniqueIoc(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	if (context) {
		IOC_REQUEST request = *(PIOC_REQUEST)context;
		ExFreePool(context);

		if (request.BufferLength >= sizeof(MOUNTDEV_UNIQUE_ID)) {
			((PMOUNTDEV_UNIQUE_ID)request.Buffer)->UniqueIdLength = 0;
		}

		if (request.OldRoutine && irp->StackCount > 1) {
			return request.OldRoutine(device, irp, request.OldContext);
		}
	}

	return STATUS_SUCCESS;
}

VOID MountFilter(PCONTEXT context, PVOID returnAddress, PVOID *frame, PVOID *base) {
	UNREFERENCED_PARAMETER(returnAddress);
	UNREFERENCED_PARAMETER(frame);
	UNREFERENCED_PARAMETER(base);

	PIRP irp = (PIRP)context->Rdi;
	if (!irp) {
		return;
	}

	PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(irp);
	switch (ioc->Parameters.DeviceIoControl.IoControlCode) {
		case IOCTL_MOUNTMGR_QUERY_POINTS:
			ChangeIoc(ioc, irp, MountPointsIoc);
			break;
		case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
			ChangeIoc(ioc, irp, MountUniqueIoc);
			break;
	}
}

NTSTATUS SetMountWpp() {
	UNICODE_STRING mountStr = RTL_CONSTANT_STRING(L"\\Driver\\mountmgr");
	PDRIVER_OBJECT mountObject = 0;

	NTSTATUS status = ObReferenceObjectByName(&mountStr, OBJ_CASE_INSENSITIVE, 0, 0, *IoDriverObjectType, KernelMode, 0, &mountObject);
	if (!NT_SUCCESS(status)) {
		printf("! failed to get %wZ driver object: %x !\n", &mountStr, status);
		return status;
	}

	PVOID wppGlobal = FindPatternImage(mountObject->DriverStart, "\x48\x89\x3D", "xxx");
	if (!wppGlobal) {
		printf("! failed to find %wZ WppGlobal !\n", &mountStr);

		ObDereferenceObject(mountObject);
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	PVOID wppTraceMessage = FindPatternImage(mountObject->DriverStart, "\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x05", "xxx????xxx");
	if (!wppTraceMessage) {
		printf("! failed to find %wZ WppTraceMessage !\n", &mountStr);

		ObDereferenceObject(mountObject);
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	PVOID returnAddress = FindPatternImage(mountObject->DriverStart, "\x45\x8B\xCE\xE8\x00\x00\x00\x00\x90\xE9", "xxxx????xx");
	if (!returnAddress) {
		printf("! failed to find %wZ return address !\n", &mountStr);

		ObDereferenceObject(mountObject);
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	WppSet((PBYTE)returnAddress + 8, MountFilter, RELATIVE_ADDR(wppGlobal, 7), RELATIVE_ADDR(wppTraceMessage, 7));
	
	printf("success for %wZ\n", &mountStr);
	ObDereferenceObject(mountObject);
	return STATUS_SUCCESS;
}