// Copyright (c) 2026 Randall Rosas (Slategray). All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TOPOLOGY MINIPORT & AUDIO ENDPOINT ROUTING
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "leyline_miniport.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CMiniportTopology
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CMiniportTopology::CMiniportTopology(BOOLEAN IsCapture)
    : CUnknown(nullptr)
    , m_IsCapture(IsCapture)
    , m_IsInitialized(FALSE)
    , m_Port(nullptr)
{}

CMiniportTopology::~CMiniportTopology() {}

STDMETHODIMP CMiniportTopology::NonDelegatingQueryInterface(REFIID riid, PVOID* ppvObject)
{
    if (IsEqualGUID(riid, IID_IMiniportTopology_) ||
        IsEqualGUID(riid, IID_IUnknown)           ||
        IsEqualGUID(riid, IID_IMiniport_))
    {
        *ppvObject = reinterpret_cast<PVOID>(static_cast<IMiniportTopology*>(this));
        AddRef();
        return STATUS_SUCCESS;
    }
    *ppvObject = nullptr;
    return STATUS_NOINTERFACE;
}

STDMETHODIMP CMiniportTopology::GetDescription(PPCFILTER_DESCRIPTOR* Description)
{
    if (!Description) return STATUS_INVALID_PARAMETER;
    DbgPrint("LeylineTopo: GetDescription (capture=%d)\n", (int)m_IsCapture);
    *Description = m_IsCapture ? &g_TopoCaptureFilterDescriptor : &g_TopoRenderFilterDescriptor;
    DbgPrint("LeylineTopo: GetDescription SUCCESS\n");
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportTopology::DataRangeIntersection(
    ULONG PinId, PKSDATARANGE /*DataRange*/, PKSDATARANGE /*MatchingDataRange*/,
    ULONG DataFormatSize, PVOID DataFormat, PULONG ResultantFormatSize)
{
    DbgPrint("LeylineTopo: DataRangeIntersection pin=%u\n", PinId);
    if (PinId > 1) return STATUS_INVALID_PARAMETER;

    ULONG needed = sizeof(KSDATAFORMAT);
    if (!DataFormat || DataFormatSize == 0)
    {
        if (ResultantFormatSize) *ResultantFormatSize = needed;
        return STATUS_BUFFER_TOO_SMALL;
    }
    if (DataFormatSize < needed)
    {
        if (ResultantFormatSize) *ResultantFormatSize = needed;
        return STATUS_BUFFER_TOO_SMALL;
    }

    auto *fmt = reinterpret_cast<KSDATAFORMAT*>(DataFormat);
    RtlZeroMemory(fmt, needed);
    fmt->FormatSize  = needed;
    fmt->MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    fmt->SubFormat   = KSDATAFORMAT_SUBTYPE_ANALOG;
    fmt->Specifier   = KSDATAFORMAT_SPECIFIER_NONE;

    if (ResultantFormatSize) *ResultantFormatSize = needed;
    DbgPrint("LeylineTopo: DataRangeIntersection -> SUCCESS Analog Bridge\n");
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportTopology::Init(PUNKNOWN /*UnknownAdapter*/, PRESOURCELIST /*ResourceList*/, IPortTopology* Port)
{
    DbgPrint("LeylineTopo: Init called\n");
    m_Port          = Port;
    m_IsInitialized = TRUE;
    DbgPrint("LeylineTopo: Init SUCCESS\n");
    return STATUS_SUCCESS;
}
