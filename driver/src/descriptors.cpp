// Copyright (c) 2026 Randall Rosas (Slategray). All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// KERNEL STREAMING FILTER & PIN DESCRIPTORS
// All statically-initialized descriptor tables for Wave and Topology filters.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "leyline_descriptors.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DATA RANGES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const KSDATARANGE_AUDIO_EX g_PcmDataRange =
{
    // KSDATARANGE base
    {
        sizeof(KSDATARANGE_AUDIO_EX), 0, 0, 0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    2,      // MaximumChannels
    8,      // MinimumBitsPerSample
    32,     // MaximumBitsPerSample
    8000,   // MinimumSampleFrequency
    192000  // MaximumSampleFrequency
};

const KSDATARANGE_AUDIO_EX g_FloatDataRange =
{
    {
        sizeof(KSDATARANGE_AUDIO_EX), 0, 0, 0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    2, 8, 32, 8000, 192000
};

const KSDATARANGE g_BridgeDataRange =
{
    sizeof(KSDATARANGE), 0, 0, 0,
    STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
    STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
};

const KSDATARANGE * const g_WaveDataRanges[2]   = { &g_PcmDataRange, &g_FloatDataRange };
const KSDATARANGE * const g_BridgeDataRanges[1] = { &g_BridgeDataRange };

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// INTERFACES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static const KSIDENTIFIER g_KsInterfaces[] =
{
    { STATICGUIDOF(KSINTERFACESETID_Standard), KSINTERFACE_STANDARD_STREAMING, 0 },
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PROPERTY HANDLER HELPERS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static NTSTATUS HandleBasicSupportFull(PPCPROPERTY_REQUEST Req, ULONG AccessFlags, ULONG TypeId)
{
    const ULONG fullSize = sizeof(KSPROPERTY_DESCRIPTION_EX);
    const ULONG ulongSize = sizeof(ULONG);

    if (Req->ValueSize == 0)
    {
        Req->ValueSize = fullSize;
        return STATUS_BUFFER_OVERFLOW;
    }
    if (Req->ValueSize >= fullSize)
    {
        auto *desc = reinterpret_cast<KSPROPERTY_DESCRIPTION_EX*>(Req->Value);
        if (desc)
        {
            desc->AccessFlags      = AccessFlags;
            desc->DescriptionSize  = fullSize;
            desc->PropTypeSet.Set  = KSPROPTYPESETID_GENERAL_PROP;
            desc->PropTypeSet.Id   = TypeId;
            desc->PropTypeSet.Flags = 0;
            desc->MembersListCount = 0;
            desc->Reserved         = 0;
        }
        Req->ValueSize = fullSize;
        return STATUS_SUCCESS;
    }
    if (Req->ValueSize >= ulongSize)
    {
        auto *flags = reinterpret_cast<ULONG*>(Req->Value);
        if (flags) *flags = AccessFlags;
        Req->ValueSize = ulongSize;
        return STATUS_SUCCESS;
    }
    return STATUS_BUFFER_TOO_SMALL;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PROPERTY HANDLERS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NTSTATUS ComponentIdHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest) return STATUS_INVALID_PARAMETER;

    DbgPrint("LeylineKernel: KSPROPERTY_GENERAL_COMPONENTID\n");

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        return HandleBasicSupportFull(PropertyRequest,
               KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, VT_I4);

    if (PropertyRequest->ValueSize == 0)
    {
        PropertyRequest->ValueSize = sizeof(KSCOMPONENTID);
        return STATUS_BUFFER_OVERFLOW;
    }
    if (PropertyRequest->ValueSize < sizeof(KSCOMPONENTID))
        return STATUS_BUFFER_TOO_SMALL;

    auto *id = reinterpret_cast<KSCOMPONENTID*>(PropertyRequest->Value);
    if (id)
    {
        id->Manufacturer = { 0x534C, 0x4154, 0x4547, { 0x52,0x41,0x59,0x44,0x45,0x56,0x31,0x31 } };
        id->Product      = { 0x4C45, 0x594C, 0x494E, { 0x45,0x41,0x55,0x44,0x49,0x4F,0x31,0x31 } };
        id->Component    = { 0xDEADBEEF, 0xCAFE, 0xFEED, { 0x4C,0x45,0x59,0x4C,0x49,0x4E,0x45,0x31 } };
        id->Name         = GUID_NULL;
        id->Version      = 1;
        id->Revision     = 0;
    }
    return STATUS_SUCCESS;
}

NTSTATUS JackDescriptionHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest) return STATUS_INVALID_PARAMETER;
    if (!PropertyRequest->PropertyItem) return STATUS_INVALID_PARAMETER;

    ULONG propId = PropertyRequest->PropertyItem->Id;
    ULONG pinId  = 0xFFFFFFFF;
    if (PropertyRequest->InstanceSize >= sizeof(ULONG))
        pinId = *reinterpret_cast<ULONG*>(PropertyRequest->Instance);

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        return HandleBasicSupportFull(PropertyRequest,
               KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, VT_I4);

    DbgPrint("Leyline: JackDescriptionHandler ID=%u Pin=%u\n", propId, pinId);

    if (propId == KSPROPERTY_JACK_DESCRIPTION_ID)
    {
        if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = sizeof(KSJACK_DESCRIPTION);
            return STATUS_BUFFER_OVERFLOW;
        }
        if (PropertyRequest->ValueSize < sizeof(KSJACK_DESCRIPTION))
            return STATUS_BUFFER_TOO_SMALL;

        auto *j = reinterpret_cast<KSJACK_DESCRIPTION*>(PropertyRequest->Value);
        if (j)
        {
            j->ChannelMapping  = 0x3;   // KSAUDIO_SPEAKER_STEREO
            j->Color           = 0;
            j->ConnectionType  = 1;     // eConnType3Point5mm
            j->GeoLocation     = 1;     // eGeoLocRear
            j->GenLocation     = 0;
            j->PortConnection  = 0;
            j->IsConnected     = TRUE;
        }
        return STATUS_SUCCESS;
    }
    else if (propId == KSPROPERTY_JACK_DESCRIPTION2_ID)
    {
        if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = sizeof(KSJACK_DESCRIPTION2);
            return STATUS_BUFFER_OVERFLOW;
        }
        if (PropertyRequest->ValueSize < sizeof(KSJACK_DESCRIPTION2))
            return STATUS_BUFFER_TOO_SMALL;

        auto *j2 = reinterpret_cast<KSJACK_DESCRIPTION2*>(PropertyRequest->Value);
        if (j2) { j2->DeviceStateInfo = 0; j2->JackCapabilities = 0; }
        return STATUS_SUCCESS;
    }
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS VolumeHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest) return STATUS_INVALID_PARAMETER;

    struct VOL_BASIC
    {
        KSPROPERTY_DESCRIPTION_EX  desc;
        KSPROPERTY_MEMBERSHEADER_EX hdr;
        KSPROPERTY_STEPPING_LONG_EX stepping;
    };

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        const ULONG fullSize  = sizeof(VOL_BASIC);
        const ULONG ulongSize = sizeof(ULONG);

        if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = fullSize;
            return STATUS_BUFFER_OVERFLOW;
        }
        if (PropertyRequest->ValueSize >= fullSize)
        {
            auto *v = reinterpret_cast<VOL_BASIC*>(PropertyRequest->Value);
            if (v)
            {
                v->desc.AccessFlags      = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT;
                v->desc.DescriptionSize  = fullSize;
                v->desc.PropTypeSet.Set  = KSPROPTYPESETID_GENERAL_PROP;
                v->desc.PropTypeSet.Id   = VT_I4;
                v->desc.PropTypeSet.Flags = 0;
                v->desc.MembersListCount = 1;
                v->desc.Reserved         = 0;
                v->hdr.MembersFlags  = KSPROPERTY_MEMBER_STEPPEDRANGES;
                v->hdr.MembersSize   = sizeof(KSPROPERTY_STEPPING_LONG_EX);
                v->hdr.MembersCount  = 1;
                v->hdr.Flags         = 0;
                v->stepping.SignedMinimum  = -96 * 0x10000;
                v->stepping.SignedMaximum  = 0;
                v->stepping.SteppingDelta  = 0x10000;
                v->stepping.Reserved       = 0;
            }
            PropertyRequest->ValueSize = fullSize;
            return STATUS_SUCCESS;
        }
        if (PropertyRequest->ValueSize >= ulongSize)
        {
            auto *f = reinterpret_cast<ULONG*>(PropertyRequest->Value);
            if (f) *f = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT;
            PropertyRequest->ValueSize = ulongSize;
            return STATUS_SUCCESS;
        }
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (PropertyRequest->ValueSize == 0)
    {
        PropertyRequest->ValueSize = sizeof(LONG);
        return STATUS_BUFFER_OVERFLOW;
    }
    if (PropertyRequest->ValueSize < sizeof(LONG))
        return STATUS_BUFFER_TOO_SMALL;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        auto *val = reinterpret_cast<LONG*>(PropertyRequest->Value);
        if (val) *val = 0;  // 0 dB default
    }
    return STATUS_SUCCESS;
}

NTSTATUS MuteHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest) return STATUS_INVALID_PARAMETER;

    ULONG rwFlags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        return HandleBasicSupportFull(PropertyRequest, rwFlags, VT_BOOL);

    if (PropertyRequest->ValueSize == 0) { PropertyRequest->ValueSize = sizeof(LONG); return STATUS_BUFFER_OVERFLOW; }
    if (PropertyRequest->ValueSize < sizeof(LONG)) return STATUS_BUFFER_TOO_SMALL;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        auto *val = reinterpret_cast<LONG*>(PropertyRequest->Value);
        if (val) *val = 0;  // Unmuted by default.
    }
    return STATUS_SUCCESS;
}

NTSTATUS PinCategoryHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest || !PropertyRequest->PropertyItem) return STATUS_INVALID_PARAMETER;
    DbgPrint("Leyline: PinCategoryHandler ID=%u\n", PropertyRequest->PropertyItem->Id);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS PinNameHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest || !PropertyRequest->PropertyItem) return STATUS_INVALID_PARAMETER;
    DbgPrint("Leyline: PinNameHandler ID=%u\n", PropertyRequest->PropertyItem->Id);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS ProposedFormatHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest || !PropertyRequest->PropertyItem) return STATUS_INVALID_PARAMETER;

    DbgPrint("Leyline: ProposedFormatHandler\n");

    ULONG rwFlags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        return HandleBasicSupportFull(PropertyRequest, rwFlags, VT_I4);

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        return STATUS_SUCCESS;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        // Return a fixed 48 kHz 16-bit stereo extensible format.
        struct KSDATAFORMAT_WAVEFORMATEXTENSIBLE_EX
        {
            KSDATAFORMAT        DataFormat;
            WAVEFORMATEXTENSIBLE WaveFormatExt;
        };

        ULONG fmtSize = sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE_EX);
        if (PropertyRequest->ValueSize == 0) { PropertyRequest->ValueSize = fmtSize; return STATUS_BUFFER_OVERFLOW; }
        if (PropertyRequest->ValueSize < fmtSize) return STATUS_BUFFER_TOO_SMALL;

        auto *r = reinterpret_cast<KSDATAFORMAT_WAVEFORMATEXTENSIBLE_EX*>(PropertyRequest->Value);
        if (r)
        {
            r->DataFormat.FormatSize  = fmtSize;
            r->DataFormat.Flags       = 0;
            r->DataFormat.SampleSize  = 4;
            r->DataFormat.Reserved    = 0;
            r->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
            r->DataFormat.SubFormat   = KSDATAFORMAT_SUBTYPE_PCM;
            r->DataFormat.Specifier   = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;

            r->WaveFormatExt.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
            r->WaveFormatExt.Format.nChannels        = 2;
            r->WaveFormatExt.Format.nSamplesPerSec   = 48000;
            r->WaveFormatExt.Format.wBitsPerSample   = 16;
            r->WaveFormatExt.Format.nBlockAlign      = 4;
            r->WaveFormatExt.Format.nAvgBytesPerSec  = 48000 * 4;
            r->WaveFormatExt.Format.cbSize            = 22;
            r->WaveFormatExt.Samples.wValidBitsPerSample = 16;
            r->WaveFormatExt.dwChannelMask            = KSAUDIO_SPEAKER_STEREO;
            r->WaveFormatExt.SubFormat               = KSDATAFORMAT_SUBTYPE_PCM;
        }
        PropertyRequest->ValueSize = fmtSize;
        DbgPrint("Leyline: ProposedFormatHandler GET -> 48kHz Stereo PCM\n");
        return STATUS_SUCCESS;
    }
    return STATUS_SUCCESS;
}

NTSTATUS AudioEffectsDiscoveryHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest || !PropertyRequest->PropertyItem) return STATUS_INVALID_PARAMETER;
    DbgPrint("Leyline: AudioEffectsDiscoveryHandler ID=%u\n", PropertyRequest->PropertyItem->Id);

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        if (PropertyRequest->ValueSize < sizeof(ULONG)) { PropertyRequest->ValueSize = sizeof(ULONG); return STATUS_BUFFER_OVERFLOW; }
        auto *f = reinterpret_cast<ULONG*>(PropertyRequest->Value);
        if (f) *f = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
        PropertyRequest->ValueSize = sizeof(ULONG);
        return STATUS_SUCCESS;
    }
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS AudioModuleHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest || !PropertyRequest->PropertyItem) return STATUS_INVALID_PARAMETER;
    ULONG propId = PropertyRequest->PropertyItem->Id;
    DbgPrint("Leyline: AudioModuleHandler ID=%u\n", propId);

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        if (PropertyRequest->ValueSize < sizeof(ULONG)) { PropertyRequest->ValueSize = sizeof(ULONG); return STATUS_BUFFER_OVERFLOW; }
        auto *f = reinterpret_cast<ULONG*>(PropertyRequest->Value);
        if (f)
        {
            *f = (propId == KSPROPERTY_AUDIOMODULE_COMMAND_ID)
                 ? KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT
                 : KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
        }
        PropertyRequest->ValueSize = sizeof(ULONG);
        return STATUS_SUCCESS;
    }
    return STATUS_NOT_IMPLEMENTED;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AUTOMATION TABLES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static const PCPROPERTY_ITEM g_GeneralProperties[] =
{
    { &KSPROPSETID_General_,   KSPROPERTY_GENERAL_COMPONENTID_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, ComponentIdHandler }
};

static const PCPROPERTY_ITEM g_WaveFilterProperties[] =
{
    { &KSPROPSETID_General_, KSPROPERTY_GENERAL_COMPONENTID_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, ComponentIdHandler },
    { &KSPROPSETID_Pin_,     KSPROPERTY_PIN_PROPOSEDATAFORMAT,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT, ProposedFormatHandler },
    { &KSPROPSETID_Pin_,     KSPROPERTY_PIN_PROPOSEDATAFORMAT2,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT, ProposedFormatHandler },
    { &KSPROPSETID_Jack_,    KSPROPERTY_JACK_DESCRIPTION_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, JackDescriptionHandler },
    { &KSPROPSETID_Jack_,    KSPROPERTY_JACK_DESCRIPTION2_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, JackDescriptionHandler },
    { &KSPROPSETID_AudioEffectsDiscovery_, KSPROPERTY_AUDIOEFFECTSDISCOVERY_EFFECTSLIST_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, AudioEffectsDiscoveryHandler },
    { &KSPROPSETID_AudioModule_, KSPROPERTY_AUDIOMODULE_DESCRIPTORS_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, AudioModuleHandler },
    { &KSPROPSETID_AudioModule_, KSPROPERTY_AUDIOMODULE_COMMAND_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT, AudioModuleHandler },
    { &KSPROPSETID_AudioModule_, KSPROPERTY_AUDIOMODULE_NOTIFICATION_DEVICE_ID_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, AudioModuleHandler },
};

static const PCPROPERTY_ITEM g_TopoFilterProperties[] =
{
    { &KSPROPSETID_General_, KSPROPERTY_GENERAL_COMPONENTID_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, ComponentIdHandler },
    { &KSPROPSETID_Jack_,    KSPROPERTY_JACK_DESCRIPTION_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, JackDescriptionHandler },
    { &KSPROPSETID_Jack_,    KSPROPERTY_JACK_DESCRIPTION2_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, JackDescriptionHandler },
};

static const PCPROPERTY_ITEM g_PinProperties[] =
{
    { &KSPROPSETID_Pin_,  KSPROPERTY_PIN_CATEGORY,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, PinCategoryHandler },
    { &KSPROPSETID_Pin_,  KSPROPERTY_PIN_NAME,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, PinNameHandler },
    { &KSPROPSETID_Jack_, KSPROPERTY_JACK_DESCRIPTION_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, JackDescriptionHandler },
    { &KSPROPSETID_Jack_, KSPROPERTY_JACK_DESCRIPTION2_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, JackDescriptionHandler },
};

static const PCPROPERTY_ITEM g_VolumeProperties[] =
{
    { &KSPROPSETID_Audio_, KSPROPERTY_AUDIO_VOLUMELEVEL_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT, VolumeHandler }
};

static const PCPROPERTY_ITEM g_MuteProperties[] =
{
    { &KSPROPSETID_Audio_, KSPROPERTY_AUDIO_MUTE_ID,
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT, MuteHandler }
};

DEFINE_PCAUTOMATION_TABLE_PROP(g_ComponentAutomationTable,  g_GeneralProperties);
DEFINE_PCAUTOMATION_TABLE_PROP(g_WaveFilterAutomationTable,  g_WaveFilterProperties);
DEFINE_PCAUTOMATION_TABLE_PROP(g_TopoFilterAutomationTable,  g_TopoFilterProperties);
DEFINE_PCAUTOMATION_TABLE_PROP(g_PinAutomationTable,         g_PinProperties);
DEFINE_PCAUTOMATION_TABLE_PROP(g_VolumeAutomationTable,      g_VolumeProperties);
DEFINE_PCAUTOMATION_TABLE_PROP(g_MuteAutomationTable,        g_MuteProperties);

static const PCAUTOMATION_TABLE g_MinimalAutomationTable = { 0 };

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// NODES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static const PCNODE_DESCRIPTOR g_TopoNodes[] =
{
    { 0, &g_VolumeAutomationTable, &KSNODETYPE_VOLUME,     &KSAUDFNAME_MASTER_VOLUME_ },
    { 0, &g_MuteAutomationTable,   &KSNODETYPE_MUTE,       &KSAUDFNAME_MASTER_MUTE_  },
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PIN DESCRIPTORS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static const PCPIN_DESCRIPTOR g_WaveRenderPins[] =
{
    // Pin 0: Wave Sink (from client app)
    {
        4, 4, 1,
        &g_PinAutomationTable,
        {
            SIZEOF_ARRAY(g_KsInterfaces), g_KsInterfaces,
            0, nullptr,
            SIZEOF_ARRAY(g_WaveDataRanges), (PKSDATARANGE*)g_WaveDataRanges,
            KSPIN_DATAFLOW_IN, KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO, nullptr, 0
        }
    },
    // Pin 1: Wave Bridge (to topology)
    {
        1, 1, 1,
        &g_PinAutomationTable,
        {
            0, nullptr,
            0, nullptr,
            SIZEOF_ARRAY(g_BridgeDataRanges), (PKSDATARANGE*)g_BridgeDataRanges,
            KSPIN_DATAFLOW_OUT, KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO, nullptr, 0
        }
    },
};

static const PCPIN_DESCRIPTOR g_WaveCapturePins[] =
{
    // Pin 0: Wave Sink (to client app, data flows out)
    {
        4, 4, 1,
        &g_PinAutomationTable,
        {
            SIZEOF_ARRAY(g_KsInterfaces), g_KsInterfaces,
            0, nullptr,
            SIZEOF_ARRAY(g_WaveDataRanges), (PKSDATARANGE*)g_WaveDataRanges,
            KSPIN_DATAFLOW_OUT, KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO, nullptr, 0
        }
    },
    // Pin 1: Wave Bridge (from topology)
    {
        1, 1, 1,
        &g_PinAutomationTable,
        {
            0, nullptr,
            0, nullptr,
            SIZEOF_ARRAY(g_BridgeDataRanges), (PKSDATARANGE*)g_BridgeDataRanges,
            KSPIN_DATAFLOW_IN, KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO, nullptr, 0
        }
    },
};

static const PCPIN_DESCRIPTOR g_TopoRenderPins[] =
{
    // Pin 0: Bridge input from wave
    {
        1, 1, 1,
        &g_PinAutomationTable,
        {
            0, nullptr, 0, nullptr,
            SIZEOF_ARRAY(g_BridgeDataRanges), (PKSDATARANGE*)g_BridgeDataRanges,
            KSPIN_DATAFLOW_IN, KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO, nullptr, 0
        }
    },
    // Pin 1: Speaker output
    {
        1, 1, 1,
        &g_PinAutomationTable,
        {
            0, nullptr, 0, nullptr,
            SIZEOF_ARRAY(g_BridgeDataRanges), (PKSDATARANGE*)g_BridgeDataRanges,
            KSPIN_DATAFLOW_OUT, KSPIN_COMMUNICATION_NONE,
            &KSNODETYPE_SPEAKER, nullptr, 0
        }
    },
};

static const PCPIN_DESCRIPTOR g_TopoCapturePins[] =
{
    // Pin 0: Microphone input
    {
        1, 1, 1,
        &g_PinAutomationTable,
        {
            0, nullptr, 0, nullptr,
            SIZEOF_ARRAY(g_BridgeDataRanges), (PKSDATARANGE*)g_BridgeDataRanges,
            KSPIN_DATAFLOW_IN, KSPIN_COMMUNICATION_NONE,
            &KSNODETYPE_MICROPHONE, nullptr, 0
        }
    },
    // Pin 1: Bridge output to wave
    {
        1, 1, 1,
        &g_PinAutomationTable,
        {
            0, nullptr, 0, nullptr,
            SIZEOF_ARRAY(g_BridgeDataRanges), (PKSDATARANGE*)g_BridgeDataRanges,
            KSPIN_DATAFLOW_OUT, KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO, nullptr, 0
        }
    },
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CONNECTIONS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static const PCCONNECTION_DESCRIPTOR g_WaveConnections[] =
{
    // Sink (0) -> Bridge (1)
    { PCFILTER_NODE, KSPIN_WAVE_SINK, PCFILTER_NODE, KSPIN_WAVE_BRIDGE }
};

static const PCCONNECTION_DESCRIPTOR g_WaveCaptureConnections[] =
{
    // Bridge (1) -> Sink (0)
    { PCFILTER_NODE, KSPIN_WAVE_BRIDGE, PCFILTER_NODE, KSPIN_WAVE_SINK }
};

static const PCCONNECTION_DESCRIPTOR g_TopoConnections[] =
{
    { PCFILTER_NODE, KSPIN_TOPO_BRIDGE, 0,              1 },  // Bridge -> Volume in
    { 0,             0,                 1,              1 },  // Volume out -> Mute in
    { 1,             0,                 PCFILTER_NODE,  KSPIN_TOPO_LINEOUT } // Mute out -> LineOut
};

static const PCCONNECTION_DESCRIPTOR g_TopoCaptureConnections[] =
{
    { PCFILTER_NODE, 0, PCFILTER_NODE, 1 }  // Mic -> Bridge
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CATEGORIES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static const GUID g_TopoFilterCategories[]      = { STATICGUIDOF(KSCATEGORY_AUDIO), STATICGUIDOF(KSCATEGORY_TOPOLOGY) };
static const GUID g_WaveRenderCategories[]       = { STATICGUIDOF(KSCATEGORY_AUDIO), STATICGUIDOF(KSCATEGORY_RENDER), STATICGUIDOF(KSCATEGORY_REALTIME) };
static const GUID g_WaveCaptureCategories[]      = { STATICGUIDOF(KSCATEGORY_AUDIO), STATICGUIDOF(KSCATEGORY_CAPTURE), STATICGUIDOF(KSCATEGORY_REALTIME) };

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// FILTER DESCRIPTORS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const PCFILTER_DESCRIPTOR g_WaveRenderFilterDescriptor =
{
    0, &g_WaveFilterAutomationTable,
    sizeof(PCPIN_DESCRIPTOR), SIZEOF_ARRAY(g_WaveRenderPins), g_WaveRenderPins,
    0, 0, nullptr,
    SIZEOF_ARRAY(g_WaveConnections), g_WaveConnections,
    SIZEOF_ARRAY(g_WaveRenderCategories), g_WaveRenderCategories
};

const PCFILTER_DESCRIPTOR g_WaveCaptureFilterDescriptor =
{
    0, &g_WaveFilterAutomationTable,
    sizeof(PCPIN_DESCRIPTOR), SIZEOF_ARRAY(g_WaveCapturePins), g_WaveCapturePins,
    0, 0, nullptr,
    SIZEOF_ARRAY(g_WaveCaptureConnections), g_WaveCaptureConnections,
    SIZEOF_ARRAY(g_WaveCaptureCategories), g_WaveCaptureCategories
};

const PCFILTER_DESCRIPTOR g_TopoRenderFilterDescriptor =
{
    0, &g_TopoFilterAutomationTable,
    sizeof(PCPIN_DESCRIPTOR), SIZEOF_ARRAY(g_TopoRenderPins), g_TopoRenderPins,
    sizeof(PCNODE_DESCRIPTOR), SIZEOF_ARRAY(g_TopoNodes), g_TopoNodes,
    SIZEOF_ARRAY(g_TopoConnections), g_TopoConnections,
    SIZEOF_ARRAY(g_TopoFilterCategories), g_TopoFilterCategories
};

const PCFILTER_DESCRIPTOR g_TopoCaptureFilterDescriptor =
{
    0, &g_TopoFilterAutomationTable,
    sizeof(PCPIN_DESCRIPTOR), SIZEOF_ARRAY(g_TopoCapturePins), g_TopoCapturePins,
    0, 0, nullptr,
    SIZEOF_ARRAY(g_TopoCaptureConnections), g_TopoCaptureConnections,
    SIZEOF_ARRAY(g_TopoFilterCategories), g_TopoFilterCategories
};
