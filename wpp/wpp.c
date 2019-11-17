#include "stdafx.h"

DEVICE_OBJECT fakeWppGlobal = {
	.Timer = (PIO_TIMER)WPP_FLAG_ALL,
};

struct {
	WPP Buffer[0x100];
	ULONG Length;
} wpps = { 0 };

VOID WppSet(PVOID returnAddress, WPP_FILTER filter, PDEVICE_OBJECT *wppGlobal, PVOID *wppTraceMessage) {
	PWPP wpp = &wpps.Buffer[wpps.Length++];

	wpp->ReturnAddress = returnAddress;
	wpp->Filter = filter;

	wpp->WppGlobal = wppGlobal;
	wpp->WppTraceMessage = wppTraceMessage;

	wpp->WppGlobalOriginal = InterlockedExchangePointer(wppGlobal, &fakeWppGlobal);
	wpp->WppTraceMessageOriginal = InterlockedExchangePointer(wppTraceMessage, (PVOID)WppTraceMessage);
}

VOID WppUndo() {
	for (ULONG i = 0; i < wpps.Length; ++i) {
		PWPP wpp = &wpps.Buffer[i];

		InterlockedExchangePointer(wpp->WppGlobal, wpp->WppGlobalOriginal);
		InterlockedExchangePointer(wpp->WppTraceMessage, wpp->WppTraceMessageOriginal);
	}
}

ULONG WppTraceMessage(VOID *LoggerHandle, ULONG MessageFlags, LPCGUID MessageGuid, USHORT MessageNumber, ...) {
	UNREFERENCED_PARAMETER(LoggerHandle);
	UNREFERENCED_PARAMETER(MessageFlags);
	UNREFERENCED_PARAMETER(MessageGuid);
	UNREFERENCED_PARAMETER(MessageNumber);

	CONTEXT context = { 0 };
	RtlCaptureContext(&context);

	PVOID returnAddress = 0;
	if (!RtlCaptureStackBackTrace(2, 1, &returnAddress, 0)) {
		return 0;
	}

	for (ULONG i = 0; i < wpps.Length; ++i) {
		PWPP wpp = &wpps.Buffer[i];
		if (wpp->ReturnAddress == returnAddress) {
			wpp->Filter(&context, returnAddress, (PVOID *)_AddressOfReturnAddress(), ((PVOID *)PsGetCurrentThreadStackBase()) - 1);

			break;
		}
	}

	return 0;
}