#include "stdafx.h"

VOID SwapEndianess(PCHAR dest, PCHAR src) {
	for (size_t i = 0, l = strlen(src); i < l; i += 2) {
		dest[i] = src[i + 1];
		dest[i + 1] = src[i];
	}
}

VOID ChangeIoc(PIO_STACK_LOCATION ioc, PIRP irp, PIO_COMPLETION_ROUTINE routine) {
	PIOC_REQUEST request = (PIOC_REQUEST)ExAllocatePool(NonPagedPool, sizeof(IOC_REQUEST));
	if (!request) {
		printf("! failed to allocate IOC_REQUEST !\n");
		return;
	}

	request->Buffer = irp->AssociatedIrp.SystemBuffer;
	request->BufferLength = ioc->Parameters.DeviceIoControl.OutputBufferLength;
	request->OldContext = ioc->Context;
	request->OldRoutine = ioc->CompletionRoutine;

	ioc->Control = SL_INVOKE_ON_SUCCESS;
	ioc->Context = request;
	ioc->CompletionRoutine = routine;
}

BOOL CheckMask(PCHAR base, PCHAR pattern, PCHAR mask) {
	for (; *mask; ++base, ++pattern, ++mask) {
		if (*mask == 'x' && *base != *pattern) {
			return FALSE;
		}
	}

	return TRUE;
}

PVOID FindPattern(PCHAR base, DWORD length, PCHAR pattern, PCHAR mask) {
	length -= (DWORD)strlen(mask);
	for (DWORD i = 0; i <= length; ++i) {
		PVOID addr = &base[i];
		if (CheckMask(addr, pattern, mask)) {
			return addr;
		}
	}

	return 0;
}

PVOID FindPatternImage(PCHAR base, PCHAR pattern, PCHAR mask) {
	PVOID match = 0;

	PIMAGE_NT_HEADERS headers = (PIMAGE_NT_HEADERS)(base + ((PIMAGE_DOS_HEADER)base)->e_lfanew);
	PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION(headers);
	for (DWORD i = 0; i < headers->FileHeader.NumberOfSections; ++i) {
		PIMAGE_SECTION_HEADER section = &sections[i];
		if (*(PINT)section->Name == 'EGAP' || memcmp(section->Name, ".text", 5) == 0) {
			match = FindPattern(base + section->VirtualAddress, section->Misc.VirtualSize, pattern, mask);
			if (match) {
				break;
			}
		}
	}

	return match;
}