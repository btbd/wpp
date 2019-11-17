#pragma once

#define WPP_FLAG_ALL (0xFFFFFFFFFFFFFFFF)

typedef VOID(*WPP_FILTER)(PCONTEXT, PVOID, PVOID, PVOID);

typedef struct _WPP {
	PVOID ReturnAddress;
	WPP_FILTER Filter;

	PDEVICE_OBJECT *WppGlobal;
	PDEVICE_OBJECT WppGlobalOriginal;

	PVOID *WppTraceMessage;
	PVOID WppTraceMessageOriginal;
} WPP, *PWPP;

VOID WppSet(PVOID returnAddress, WPP_FILTER filter, PDEVICE_OBJECT *wppGlobal, PVOID *wppTraceMessage);
VOID WppUndo();
ULONG WppTraceMessage(VOID *LoggerHandle, ULONG MessageFlags, LPCGUID MessageGuid, USHORT MessageNumber, ...);