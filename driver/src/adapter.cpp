// Copyright (c) 2026 Randall Rosas (Slategray). All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ADAPTER MANAGEMENT & PORTCLS ORCHESTRATION
// Registers subdevices, manages physical connections, creates the CDO.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "leyline_miniport.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GLOBALS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Control Device Object for IOCTL communication with user-mode (HSA/APO).
PDEVICE_OBJECT g_ControlDeviceObject    = nullptr;
PDEVICE_OBJECT g_FunctionalDeviceObject = nullptr;
ULONGLONG      g_EtwRegHandle           = 0;

// Original dispatch routines that PortCls wired in; we chain to them for non-CDO IRPs.
static PDRIVER_DISPATCH s_OriginalDispatchCreate  = nullptr;
static PDRIVER_DISPATCH s_OriginalDispatchClose   = nullptr;
static PDRIVER_DISPATCH s_OriginalDispatchControl = nullptr;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// IRP DISPATCH ROUTINES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static NTSTATUS DispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    if (DeviceObject != g_ControlDeviceObject)
    {
        if (s_OriginalDispatchCreate)
            return s_OriginalDispatchCreate(DeviceObject, Irp);
        return STATUS_DEVICE_NOT_READY;
    }
    DbgPrint("Leyline: CDO Create\n");
    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS DispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    if (DeviceObject != g_ControlDeviceObject)
    {
        if (s_OriginalDispatchClose)
            return s_OriginalDispatchClose(DeviceObject, Irp);
        return STATUS_DEVICE_NOT_READY;
    }
    DbgPrint("Leyline: CDO Close\n");
    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS DispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    if (DeviceObject != g_ControlDeviceObject)
    {
        if (s_OriginalDispatchControl)
            return s_OriginalDispatchControl(DeviceObject, Irp);
        return STATUS_DEVICE_NOT_READY;
    }

    PIO_STACK_LOCATION stack    = IoGetCurrentIrpStackLocation(Irp);
    ULONG              ioctl    = stack->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS           status   = STATUS_SUCCESS;
    ULONG_PTR          info     = 0;

    DbgPrint("Leyline: CDO IOCTL 0x%08X\n", ioctl);

    switch (ioctl)
    {
    case IOCTL_LEYLINE_GET_STATUS:
    {
        ULONG outLen = stack->Parameters.DeviceIoControl.OutputBufferLength;
        if (outLen >= sizeof(ULONG))
        {
            *reinterpret_cast<ULONG*>(Irp->AssociatedIrp.SystemBuffer) = 0x1337BEEF;
            info = sizeof(ULONG);
        }
        else
            status = STATUS_BUFFER_TOO_SMALL;
        break;
    }
    case IOCTL_LEYLINE_MAP_BUFFER:
    {
        ULONG outLen = stack->Parameters.DeviceIoControl.OutputBufferLength;
        if (outLen >= sizeof(PVOID) && g_FunctionalDeviceObject)
        {
            DeviceExtension *ext = GetDeviceExtension(g_FunctionalDeviceObject);
            *reinterpret_cast<PVOID*>(Irp->AssociatedIrp.SystemBuffer) = ext->UserMapping;
            info = sizeof(PVOID);
        }
        else
            status = g_FunctionalDeviceObject ? STATUS_BUFFER_TOO_SMALL : STATUS_DEVICE_NOT_READY;
        break;
    }
    case IOCTL_LEYLINE_MAP_PARAMS:
    {
        ULONG outLen = stack->Parameters.DeviceIoControl.OutputBufferLength;
        if (outLen >= sizeof(PVOID) && g_FunctionalDeviceObject)
        {
            DeviceExtension *ext = GetDeviceExtension(g_FunctionalDeviceObject);
            *reinterpret_cast<PVOID*>(Irp->AssociatedIrp.SystemBuffer) = ext->SharedParamsUserMapping;
            info = sizeof(PVOID);
        }
        else
            status = g_FunctionalDeviceObject ? STATUS_BUFFER_TOO_SMALL : STATUS_DEVICE_NOT_READY;
        break;
    }
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// StartDevice - PORTCLS CALLBACK
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern "C" NTSTATUS NTAPI StartDevice(PDEVICE_OBJECT DeviceObject, PIRP Irp, PRESOURCELIST ResourceList)
{
    NTSTATUS status;
    DeviceExtension *devExt = GetDeviceExtension(DeviceObject);

    DbgPrint("Leyline: StartDevice (FDO: %p)\n", DeviceObject);

    // ---- WaveRender ----
    DbgPrint("Leyline: Registering WaveRender Port\n");

    PUNKNOWN renderPort = nullptr;
    status = PcNewPort(&renderPort, CLSID_PortWaveRT);
    if (!NT_SUCCESS(status)) { DbgPrint("Leyline: PcNewPort(WaveRT Render) FAILED 0x%X\n", status); return status; }

    CMiniportWaveRT *renderMiniport = new (NonPagedPool, 'LLWR') CMiniportWaveRT(FALSE, devExt);
    if (!renderMiniport) { renderPort->Release(); return STATUS_INSUFFICIENT_RESOURCES; }
    devExt->RenderMiniport = renderMiniport;

    PUNKNOWN renderMiniportUnk = nullptr;
    renderMiniport->QueryInterface(IID_IUnknown, reinterpret_cast<PVOID*>(&renderMiniportUnk));

    status = static_cast<IPortWaveRT*>(renderPort)->Init(nullptr, ResourceList, nullptr);
    // Note: PortCls's IPort::Init signature in some WDKs is:
    //   Init(DeviceObject, Irp, Miniport, UnknownAdapter, ResourceList)
    // We call through the IPortWaveRT vtable which the SDK handles.

    UNICODE_STRING renderName;
    RtlInitUnicodeString(&renderName, L"WaveRender");
    status = PcRegisterSubdevice(DeviceObject, &renderName, renderMiniportUnk);
    if (!NT_SUCCESS(status)) { DbgPrint("Leyline: PcRegisterSubdevice(WaveRender) FAILED 0x%X\n", status); return status; }
    DbgPrint("Leyline: WaveRender Subdevice Registered\n");

    // ---- WaveCapture ----
    DbgPrint("Leyline: Registering WaveCapture Port\n");

    PUNKNOWN capturePort = nullptr;
    status = PcNewPort(&capturePort, CLSID_PortWaveRT);
    if (!NT_SUCCESS(status)) return status;

    CMiniportWaveRT *captureMiniport = new (NonPagedPool, 'LLWC') CMiniportWaveRT(TRUE, devExt);
    if (!captureMiniport) { capturePort->Release(); return STATUS_INSUFFICIENT_RESOURCES; }
    devExt->CaptureMiniport = captureMiniport;

    PUNKNOWN captureMiniportUnk = nullptr;
    captureMiniport->QueryInterface(IID_IUnknown, reinterpret_cast<PVOID*>(&captureMiniportUnk));

    UNICODE_STRING captureName;
    RtlInitUnicodeString(&captureName, L"WaveCapture");
    status = PcRegisterSubdevice(DeviceObject, &captureName, captureMiniportUnk);
    if (!NT_SUCCESS(status)) { DbgPrint("Leyline: PcRegisterSubdevice(WaveCapture) FAILED 0x%X\n", status); return status; }
    DbgPrint("Leyline: WaveCapture Subdevice Registered\n");

    // ---- TopologyRender ----
    DbgPrint("Leyline: Registering TopologyRender Port\n");

    PUNKNOWN renderTopoPort = nullptr;
    status = PcNewPort(&renderTopoPort, CLSID_PortTopology);
    if (!NT_SUCCESS(status)) { DbgPrint("Leyline: PcNewPort(TopologyRender) FAILED 0x%X\n", status); return status; }

    CMiniportTopology *renderTopoMiniport = new (NonPagedPool, 'LLTR') CMiniportTopology(FALSE);
    if (!renderTopoMiniport) { renderTopoPort->Release(); return STATUS_INSUFFICIENT_RESOURCES; }
    devExt->RenderTopoMiniport = renderTopoMiniport;

    PUNKNOWN renderTopoMiniportUnk = nullptr;
    renderTopoMiniport->QueryInterface(IID_IUnknown, reinterpret_cast<PVOID*>(&renderTopoMiniportUnk));

    UNICODE_STRING topoRenderName;
    RtlInitUnicodeString(&topoRenderName, L"TopologyRender");
    status = PcRegisterSubdevice(DeviceObject, &topoRenderName, renderTopoMiniportUnk);
    if (!NT_SUCCESS(status)) { DbgPrint("Leyline: PcRegisterSubdevice(TopologyRender) FAILED 0x%X\n", status); return status; }
    DbgPrint("Leyline: TopologyRender Subdevice Registered\n");

    // Physical connection: WaveRender (pin 1) -> TopologyRender (pin 0)
    status = PcRegisterPhysicalConnection(DeviceObject, renderMiniportUnk, 1, renderTopoMiniportUnk, 0);
    if (!NT_SUCCESS(status)) { DbgPrint("Leyline: Physical Connection Wave->Topo FAILED 0x%X\n", status); return status; }
    DbgPrint("Leyline: Physical Connection Wave->Topo SUCCESS\n");

    // ---- TopologyCapture ----
    DbgPrint("Leyline: Registering TopologyCapture Port\n");

    PUNKNOWN captureTopoPort = nullptr;
    status = PcNewPort(&captureTopoPort, CLSID_PortTopology);
    if (!NT_SUCCESS(status)) { DbgPrint("Leyline: PcNewPort(TopologyCapture) FAILED 0x%X\n", status); return status; }

    CMiniportTopology *captureTopoMiniport = new (NonPagedPool, 'LLTC') CMiniportTopology(TRUE);
    if (!captureTopoMiniport) { captureTopoPort->Release(); return STATUS_INSUFFICIENT_RESOURCES; }
    devExt->CaptureTopoMiniport = captureTopoMiniport;

    PUNKNOWN captureTopoMiniportUnk = nullptr;
    captureTopoMiniport->QueryInterface(IID_IUnknown, reinterpret_cast<PVOID*>(&captureTopoMiniportUnk));

    UNICODE_STRING topoCaptureName;
    RtlInitUnicodeString(&topoCaptureName, L"TopologyCapture");
    status = PcRegisterSubdevice(DeviceObject, &topoCaptureName, captureTopoMiniportUnk);
    if (!NT_SUCCESS(status)) { DbgPrint("Leyline: PcRegisterSubdevice(TopologyCapture) FAILED 0x%X\n", status); return status; }
    DbgPrint("Leyline: TopologyCapture Subdevice Registered\n");

    // Physical connection: TopologyCapture (pin 1) -> WaveCapture (pin 1)
    status = PcRegisterPhysicalConnection(DeviceObject, captureTopoMiniportUnk, 1, captureMiniportUnk, 1);
    if (!NT_SUCCESS(status)) { DbgPrint("Leyline: Physical Connection Topo->Wave(Capture) FAILED 0x%X\n", status); return status; }
    DbgPrint("Leyline: Physical Connection Topo->Wave(Capture) SUCCESS\n");

    // ---- CDO Creation ----
    g_FunctionalDeviceObject = DeviceObject;
    if (!g_ControlDeviceObject)
    {
        UNICODE_STRING deviceName;
        RtlInitUnicodeString(&deviceName, L"\\Device\\LeylineAudio");

        status = IoCreateDevice(DeviceObject->DriverObject, sizeof(PVOID),
                                &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &g_ControlDeviceObject);
        if (NT_SUCCESS(status))
        {
            UNICODE_STRING linkName;
            RtlInitUnicodeString(&linkName, L"\\DosDevices\\LeylineAudio");
            IoCreateSymbolicLink(&linkName, &deviceName);
            DbgPrint("Leyline: CDO Ready\n");
        }
        else if (status == STATUS_OBJECT_NAME_COLLISION)
        {
            DbgPrint("Leyline: CDO already exists, skipping creation\n");
            status = STATUS_SUCCESS;
        }
    }
    else
        status = STATUS_SUCCESS;

    if (NT_SUCCESS(status))
    {
        DbgPrint("Leyline: ==============================================\n");
        DbgPrint("Leyline: StartDevice COMPLETED SUCCESSFULLY v0.1.0\n");
        DbgPrint("Leyline:   - WaveRender / WaveCapture\n");
        DbgPrint("Leyline:   - TopologyRender / TopologyCapture\n");
        DbgPrint("Leyline: ==============================================\n");
    }

    return status;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AddDevice - PORTCLS CALLBACK
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern "C" NTSTATUS NTAPI AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject)
{
    DbgPrint("Leyline: AddDevice (PDO: %p)\n", PhysicalDeviceObject);

    ULONG extensionSize = (ULONG)(PORT_CLASS_DEVICE_EXTENSION_SIZE + sizeof(DeviceExtension));
    return PcAddAdapterDevice(DriverObject, PhysicalDeviceObject, StartDevice, 10, extensionSize);
}
