#pragma once

#define LENGTH(a) (sizeof(a) / sizeof(a[0]))
#define RELATIVE_ADDR(addr, size) ((PVOID)((PBYTE)addr + *(PINT)((PBYTE)addr + (size - (INT)sizeof(INT))) + size))

typedef struct _IOC_REQUEST {
	PVOID Buffer;
	ULONG BufferLength;
	PVOID OldContext;
	PIO_COMPLETION_ROUTINE OldRoutine;
} IOC_REQUEST, *PIOC_REQUEST;

VOID SwapEndianess(PCHAR dest, PCHAR src);
VOID ChangeIoc(PIO_STACK_LOCATION ioc, PIRP irp, PIO_COMPLETION_ROUTINE routine);
PVOID FindPattern(PCHAR base, DWORD length, PCHAR pattern, PCHAR mask);
PVOID FindPatternImage(PCHAR base, PCHAR pattern, PCHAR mask);