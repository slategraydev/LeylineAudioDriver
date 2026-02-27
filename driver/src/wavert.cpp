// Copyright (c) 2026 Randall Rosas (Slategray). All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// WAVERT MINIPORT & STREAM IMPLEMENTATION
// Handles audio format negotiation, streaming, and buffer lifecycle.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "leyline_miniport.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CMiniportWaveRTStream
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CMiniportWaveRTStream::CMiniportWaveRTStream(DeviceExtension* DevExt)
    : CUnknown(nullptr)
    , m_State(KSSTATE_STOP)
    , m_Mdl(nullptr)
    , m_Mapping(nullptr)
    , m_IsCapture(FALSE)
    , m_OwnsMdl(FALSE)
    , m_StartTime(0)
    , m_ByteRate(48000 * 4)
    , m_Frequency(0)
    , m_DevExt(DevExt)
{
    LARGE_INTEGER freq = {};
    KeQueryPerformanceCounter(&freq);
    m_Frequency = freq.QuadPart;
}

CMiniportWaveRTStream::~CMiniportWaveRTStream()
{
    if (m_OwnsMdl && m_Mdl)
    {
        if (m_Mapping)
            MmUnmapLockedPages(m_Mapping, m_Mdl);
        MmFreePagesFromMdl(m_Mdl);
        IoFreeMdl(m_Mdl);
        m_Mdl     = nullptr;
        m_Mapping = nullptr;
    }
}

NTSTATUS CMiniportWaveRTStream::Init(ULONG /*PinId*/, BOOLEAN Capture, PKSDATAFORMAT Format)
{
    m_IsCapture = Capture;
    if (Format)
    {
        auto *wfx = reinterpret_cast<KSDATAFORMAT*>(Format);
        // Walk past the KSDATAFORMAT header to reach WAVEFORMATEX.
        auto *wave = reinterpret_cast<WAVEFORMATEX*>(wfx + 1);
        m_ByteRate = wave->nAvgBytesPerSec;
    }
    DbgPrint("LeylineWaveRT: Stream Init (capture=%d, byteRate=%u)\n", (int)m_IsCapture, m_ByteRate);
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRTStream::NonDelegatingQueryInterface(REFIID riid, PVOID* ppvObject)
{
    if (IsEqualGUID(riid, IID_IMiniportWaveRTStream_) || IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObject = reinterpret_cast<PVOID>(static_cast<IMiniportWaveRTStream*>(this));
        AddRef();
        return STATUS_SUCCESS;
    }
    return STATUS_NOINTERFACE;
}

STDMETHODIMP CMiniportWaveRTStream::SetFormat(PKSDATAFORMAT /*DataFormat*/)
{
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRTStream::SetState(KSSTATE State)
{
    m_State = State;
    if (State == KSSTATE_STOP)
        m_StartTime = 0;
    else if (State == KSSTATE_RUN)
        m_StartTime = KeQueryPerformanceCounter(nullptr).QuadPart;
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRTStream::GetPosition(ULONGLONG* Position)
{
    if (!Position) return STATUS_INVALID_PARAMETER;

    if (m_State != KSSTATE_RUN || m_StartTime == 0)
    {
        *Position = 0;
        return STATUS_SUCCESS;
    }

    LONGLONG now     = KeQueryPerformanceCounter(nullptr).QuadPart;
    LONGLONG elapsed = now - m_StartTime;
    ULONGLONG bytes  = WaveRTMath::TicksToBytes(elapsed, m_ByteRate, m_Frequency);

    SIZE_T size = m_Buffer.GetSize();
    *Position   = (size > 0) ? (bytes % (ULONGLONG)size) : 0;
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRTStream::AllocateAudioBuffer(
    ULONG RequestedSize, PMDL* AudioBufferMdl,
    ULONG* ActualSize, ULONG* OffsetFromFirstPage,
    MEMORY_CACHING_TYPE* CacheType)
{
    if (m_Mdl) return STATUS_ALREADY_COMMITTED;

    PHYSICAL_ADDRESS low  = { 0 };
    PHYSICAL_ADDRESS high = { 0 };
    PHYSICAL_ADDRESS skip = { 0 };
    high.LowPart = 0xFFFFFFFF;

    PMDL mdl = MmAllocatePagesForMdlEx(low, high, skip, RequestedSize,
                                        MmCached, MM_ALLOCATE_FULLY_REQUIRED);
    if (!mdl)
    {
        // Fall back to the loopback buffer managed by the adapter.
        if (m_DevExt && m_DevExt->LoopbackMdl)
        {
            m_Mdl     = m_DevExt->LoopbackMdl;
            m_Mapping = m_DevExt->LoopbackBuffer;
            m_Buffer.Init(m_DevExt->LoopbackBuffer, m_DevExt->LoopbackSize);
            m_OwnsMdl = FALSE;
            if (AudioBufferMdl)     *AudioBufferMdl     = m_Mdl;
            if (ActualSize)         *ActualSize         = (ULONG)m_DevExt->LoopbackSize;
            if (OffsetFromFirstPage) *OffsetFromFirstPage = 0;
            if (CacheType)          *CacheType          = MmCached;
            return STATUS_SUCCESS;
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_Mapping = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached,
                                             nullptr, FALSE, NormalPagePriority);
    if (!m_Mapping)
    {
        IoFreeMdl(mdl);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_Mdl     = mdl;
    m_OwnsMdl = TRUE;
    m_Buffer.Init(reinterpret_cast<PUCHAR>(m_Mapping), RequestedSize);

    if (AudioBufferMdl)      *AudioBufferMdl      = m_Mdl;
    if (ActualSize)          *ActualSize          = RequestedSize;
    if (OffsetFromFirstPage) *OffsetFromFirstPage = 0;
    if (CacheType)           *CacheType           = MmCached;
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRTStream::FreeAudioBuffer(PMDL /*AudioBufferMdl*/, ULONG /*BufferSize*/)
{
    // Cleanup is deferred to the destructor to honour PortCls ref-counting.
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRTStream::GetHWLatency(ULONGLONG* FifoSize, ULONGLONG* ChipsetDelay, ULONGLONG* CodecDelay)
{
    if (FifoSize)     *FifoSize     = 0;
    if (ChipsetDelay) *ChipsetDelay = 0;
    if (CodecDelay)   *CodecDelay   = 0;
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRTStream::GetPositionRegister(KSRTAUDIO_HWREGISTER* /*Register*/)
{
    return STATUS_UNSUCCESSFUL;
}

STDMETHODIMP CMiniportWaveRTStream::GetClockRegister(KSRTAUDIO_HWREGISTER* /*Register*/)
{
    return STATUS_UNSUCCESSFUL;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CMiniportWaveRT
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CMiniportWaveRT::CMiniportWaveRT(BOOLEAN IsCapture, DeviceExtension* DevExt)
    : CUnknown(nullptr)
    , m_IsCapture(IsCapture)
    , m_IsInitialized(FALSE)
    , m_DevExt(DevExt)
{}

CMiniportWaveRT::~CMiniportWaveRT() {}

STDMETHODIMP CMiniportWaveRT::NonDelegatingQueryInterface(REFIID riid, PVOID* ppvObject)
{
    if (IsEqualGUID(riid, IID_IMiniportWaveRT_) || IsEqualGUID(riid, IID_IUnknown) || IsEqualGUID(riid, IID_IMiniport_))
    {
        *ppvObject = reinterpret_cast<PVOID>(static_cast<IMiniportWaveRT*>(this));
        AddRef();
        return STATUS_SUCCESS;
    }
    *ppvObject = nullptr;
    return STATUS_NOINTERFACE;
}

STDMETHODIMP CMiniportWaveRT::GetDescription(PPCFILTER_DESCRIPTOR* Description)
{
    if (!Description) return STATUS_INVALID_PARAMETER;
    DbgPrint("LeylineWaveRT: GetDescription (capture=%d)\n", (int)m_IsCapture);
    *Description = m_IsCapture ? &g_WaveCaptureFilterDescriptor : &g_WaveRenderFilterDescriptor;
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRT::DataRangeIntersection(
    ULONG PinId, PKSDATARANGE DataRange, PKSDATARANGE /*MatchingDataRange*/,
    ULONG DataFormatSize, PVOID DataFormat, PULONG ResultantFormatSize)
{
    if (!DataRange) return STATUS_INVALID_PARAMETER;

    DbgPrint("LeylineWaveRT: DataRangeIntersection Pin=%u SubFormat=%08lX\n",
             PinId, DataRange->SubFormat.Data1);

    if (!IsEqualGUID(DataRange->MajorFormat, KSDATAFORMAT_TYPE_AUDIO))
        return STATUS_NO_MATCH;
    if (!IsEqualGUID(DataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
        return STATUS_NO_MATCH;

    BOOLEAN isPCM   = IsEqualGUID(DataRange->SubFormat, KSDATAFORMAT_SUBTYPE_PCM);
    BOOLEAN isFloat = IsEqualGUID(DataRange->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

    if (!isPCM && !isFloat) return STATUS_NO_MATCH;

    struct KSDATAFORMAT_WAVEFORMATEX_EX { KSDATAFORMAT df; WAVEFORMATEX wfx; };
    ULONG fmtSize = sizeof(KSDATAFORMAT_WAVEFORMATEX_EX);

    if (DataFormatSize == 0)
    {
        if (ResultantFormatSize) *ResultantFormatSize = fmtSize;
        return STATUS_BUFFER_TOO_SMALL;
    }
    if (DataFormatSize < fmtSize) return STATUS_BUFFER_TOO_SMALL;

    auto *r = reinterpret_cast<KSDATAFORMAT_WAVEFORMATEX_EX*>(DataFormat);
    r->df.FormatSize  = fmtSize;
    r->df.Flags       = 0;
    r->df.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    r->df.SubFormat   = DataRange->SubFormat;
    r->df.Specifier   = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;

    r->wfx.wFormatTag      = isPCM ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT;
    r->wfx.nChannels       = 2;
    r->wfx.nSamplesPerSec  = 48000;
    r->wfx.wBitsPerSample  = isPCM ? 16 : 32;
    r->wfx.nBlockAlign     = r->wfx.nChannels * r->wfx.wBitsPerSample / 8;
    r->wfx.nAvgBytesPerSec = r->wfx.nSamplesPerSec * r->wfx.nBlockAlign;
    r->wfx.cbSize          = 0;
    r->df.SampleSize       = r->wfx.nBlockAlign;

    if (ResultantFormatSize) *ResultantFormatSize = fmtSize;

    DbgPrint("LeylineWaveRT: DataRangeIntersection -> SUCCESS 48kHz Stereo\n");
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRT::Init(PUNKNOWN /*UnknownAdapter*/, PRESOURCELIST /*ResourceList*/, IPortWaveRT* /*Port*/)
{
    DbgPrint("LeylineWaveRT: Init (capture=%d)\n", (int)m_IsCapture);
    m_IsInitialized = TRUE;
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRT::GetDeviceDescription(PDEVICE_DESCRIPTION /*DeviceDescription*/)
{
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRT::NewStream(
    PMINIPORTWAVERTAINSTREAM* Stream, PUNKNOWN /*OuterUnknown*/, POOL_TYPE /*PoolType*/,
    ULONG PinId, BOOLEAN Capture, PKSDATAFORMAT DataFormat, PDRMRIGHTS /*DrmRights*/)
{
    if (!Stream) return STATUS_INVALID_PARAMETER;
    if (!m_IsInitialized) return STATUS_DEVICE_NOT_READY;

    DbgPrint("LeylineWaveRT: NewStream (pin=%u, capture=%d)\n", PinId, (int)Capture);

    CMiniportWaveRTStream *stream = new (NonPagedPool, 'LLWS') CMiniportWaveRTStream(m_DevExt);
    if (!stream) return STATUS_INSUFFICIENT_RESOURCES;

    NTSTATUS status = stream->Init(PinId, Capture, DataFormat);
    if (!NT_SUCCESS(status))
    {
        delete stream;
        return status;
    }

    *Stream = reinterpret_cast<PMINIPORTWAVERTAINSTREAM>(stream);
    return STATUS_SUCCESS;
}
