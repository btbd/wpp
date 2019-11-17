#include "stdafx.h"

VOID DriverUnload(PDRIVER_OBJECT driver) {
	UNREFERENCED_PARAMETER(driver);

	WppUndo();
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registryPath) {
	UNREFERENCED_PARAMETER(registryPath);

	driver->DriverUnload = DriverUnload;

	NTSTATUS status = STATUS_SUCCESS;
	if (!NT_SUCCESS(status = SetDiskWpp())) {
		return status;
	}

	if (!NT_SUCCESS(status = SetMountWpp())) {
		return status;
	}

	return status;
}