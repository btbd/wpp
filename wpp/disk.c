#include "stdafx.h"

static CHAR *SERIAL = "123456789";

NTSTATUS StorageQueryIoc(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	if (context) {
		IOC_REQUEST request = *(PIOC_REQUEST)context;
		ExFreePool(context);

		if (request.BufferLength >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
			PSTORAGE_DEVICE_DESCRIPTOR desc = (PSTORAGE_DEVICE_DESCRIPTOR)request.Buffer;
			ULONG offset = desc->SerialNumberOffset;
			if (offset && offset < request.BufferLength) {
				strcpy((PCHAR)desc + offset, SERIAL);
			}
		}

		if (request.OldRoutine && irp->StackCount > 1) {
			return request.OldRoutine(device, irp, request.OldContext);
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS AtaPassIoc(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	if (context) {
		IOC_REQUEST request = *(PIOC_REQUEST)context;
		ExFreePool(context);

		if (request.BufferLength >= sizeof(ATA_PASS_THROUGH_EX) + sizeof(PIDENTIFY_DEVICE_DATA)) {
			PATA_PASS_THROUGH_EX pte = (PATA_PASS_THROUGH_EX)request.Buffer;
			ULONG offset = (ULONG)pte->DataBufferOffset;
			if (offset && offset < request.BufferLength) {
				PCHAR serial = (PCHAR)((PIDENTIFY_DEVICE_DATA)((PBYTE)request.Buffer + offset))->SerialNumber;
				SwapEndianess(serial, SERIAL);
			}
		}

		if (request.OldRoutine && irp->StackCount > 1) {
			return request.OldRoutine(device, irp, request.OldContext);
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS SmartDataIoc(PDEVICE_OBJECT device, PIRP irp, PVOID context) {
	if (context) {
		IOC_REQUEST request = *(PIOC_REQUEST)context;
		ExFreePool(context);

		if (request.BufferLength >= sizeof(SENDCMDOUTPARAMS)) {
			PCHAR serial = ((PIDSECTOR)((PSENDCMDOUTPARAMS)request.Buffer)->bBuffer)->sSerialNumber;
			SwapEndianess(serial, SERIAL);
		}

		if (request.OldRoutine && irp->StackCount > 1) {
			return request.OldRoutine(device, irp, request.OldContext);
		}
	}

	return STATUS_SUCCESS;
}

VOID DiskFilter(PCONTEXT context, PVOID returnAddress, PVOID *frame, PVOID *base) {
	UNREFERENCED_PARAMETER(context);

	for (; *frame != returnAddress; ++frame) {
		if (frame >= base) {
			return;
		}
	}

	PIRP irp = *(frame + 6);
	if (!irp) {
		return;
	}

	PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(irp);
	switch (ioc->Parameters.DeviceIoControl.IoControlCode) {
		case IOCTL_STORAGE_QUERY_PROPERTY:
			if (((PSTORAGE_PROPERTY_QUERY)irp->AssociatedIrp.SystemBuffer)->PropertyId == StorageDeviceProperty) {
				ChangeIoc(ioc, irp, StorageQueryIoc);
			}
			break;
		case IOCTL_ATA_PASS_THROUGH:
			ChangeIoc(ioc, irp, AtaPassIoc);
			break;
		case SMART_RCV_DRIVE_DATA:
			ChangeIoc(ioc, irp, SmartDataIoc);
			break;
	}
}

NTSTATUS SetDiskWpp() {
	UNICODE_STRING diskStr = RTL_CONSTANT_STRING(L"\\Driver\\Disk");
	PDRIVER_OBJECT diskObject = 0;

	NTSTATUS status = ObReferenceObjectByName(&diskStr, OBJ_CASE_INSENSITIVE, 0, 0, *IoDriverObjectType, KernelMode, 0, &diskObject);
	if (!NT_SUCCESS(status)) {
		printf("! failed to get %wZ driver object: %x !\n", &diskStr, status);
		return status;
	}

	PVOID wppGlobal = FindPatternImage(diskObject->DriverStart, "\x48\x89\x3D", "xxx");
	if (!wppGlobal) {
		printf("! failed to find %wZ WppGlobal !\n", &diskStr);

		ObDereferenceObject(diskObject);
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	PVOID wppTraceMessage = FindPatternImage(diskObject->DriverStart, "\x48\x8B\x05\x00\x00\x00\x00\x48\x83", "xxx????xx");
	if (!wppTraceMessage) {
		printf("! failed to find %wZ WppTraceMessage !\n", &diskStr);

		ObDereferenceObject(diskObject);
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	PVOID returnAddress = FindPatternImage(diskObject->DriverStart, "\x90\xE9\x00\x00\x00\x00\x48\x8D\x54", "xx????xxx");
	if (!returnAddress) {
		printf("! failed to find %wZ return address !\n", &diskStr);

		ObDereferenceObject(diskObject);
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	WppSet(returnAddress, DiskFilter, RELATIVE_ADDR(wppGlobal, 7), RELATIVE_ADDR(wppTraceMessage, 7));

	printf("success for %wZ\n", &diskStr);
	ObDereferenceObject(diskObject);
	return STATUS_SUCCESS;
}