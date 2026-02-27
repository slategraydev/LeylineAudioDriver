// Copyright (c) 2026 Randall Rosas (Slategray). All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DRIVER ENTRY POINT
// Wires PortCls into the driver object, then hooks key dispatch routines.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "leyline_miniport.h"

extern PDEVICE_OBJECT g_ControlDeviceObject;
extern ULONGLONG      g_EtwRegHandle;
extern "C" NTSTATUS NTAPI AddDevice(PDRIVER_OBJECT, PDEVICE_OBJECT);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DriverUnload
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static void NTAPI DriverUnload(PDRIVER_OBJECT /*DriverObject*/)
{
    DbgPrint("Leyline: DriverUnload\n");

    if (g_ControlDeviceObject)
    {
        UNICODE_STRING linkName;
        RtlInitUnicodeString(&linkName, L"\\DosDevices\\LeylineAudio");
        IoDeleteSymbolicLink(&linkName);
        IoDeleteDevice(g_ControlDeviceObject);
        g_ControlDeviceObject = nullptr;
        DbgPrint("Leyline: CDO and Symbolic Link Cleaned Up\n");
    }

    if (g_EtwRegHandle)
    {
        EtwUnregister(g_EtwRegHandle);
        g_EtwRegHandle = 0;
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DriverEntry
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    DbgPrint("Leyline: DriverEntry v0.1.0 (REBUILT)\n");

    // Register ETW provider for structured tracing.
    EtwRegister(&ETW_PROVIDER_GUID, nullptr, nullptr, &g_EtwRegHandle);

    DriverObject->DriverUnload = DriverUnload;

    NTSTATUS status = PcInitializeAdapterDriver(DriverObject, RegistryPath, AddDevice);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("Leyline: PcInitializeAdapterDriver FAILED 0x%X\n", status);
        return status;
    }
    DbgPrint("Leyline: PcInitializeAdapterDriver SUCCESS\n");

    return status;
}
