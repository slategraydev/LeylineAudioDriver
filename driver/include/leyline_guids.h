// Copyright (c) 2026 Randall Rosas (Slategray). All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LEYLINE GUIDS & CONSTANTS
// All KS, PortCls, and Leyline-specific GUIDs in one place.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once
#include <guiddef.h>
#include <ks.h>
#include <ksmedia.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PORTCLS CLASS & INTERFACE IDS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// {CC9BE57A-EB9E-42B4-94FC-0CAD3DBCE7FA}
DEFINE_GUID(CLSID_PortWaveRT,
    0xCC9BE57A, 0xEB9E, 0x42B4, 0x94, 0xFC, 0x0C, 0xAD, 0x3D, 0xBC, 0xE7, 0xFA);

// {339FF909-68A9-4310-B09B-274E96EE4CBD}
DEFINE_GUID(IID_IPortWaveRT,
    0x339FF909, 0x68A9, 0x4310, 0xB0, 0x9B, 0x27, 0x4E, 0x96, 0xEE, 0x4C, 0xBD);

// {0F9FC4D6-6061-4F3C-B1FC-075E35F7960A}
DEFINE_GUID(IID_IMiniportWaveRT_,
    0x0F9FC4D6, 0x6061, 0x4F3C, 0xB1, 0xFC, 0x07, 0x5E, 0x35, 0xF7, 0x96, 0x0A);

// {000AC9AB-FAAB-4F3D-9455-6FF8306A74A0}
DEFINE_GUID(IID_IMiniportWaveRTStream_,
    0x000AC9AB, 0xFAAB, 0x4F3D, 0x94, 0x55, 0x6F, 0xF8, 0x30, 0x6A, 0x74, 0xA0);

// {831FC7BC-6347-44BC-B47B-C0C657B5BF73}
DEFINE_GUID(IID_IMiniportWaveRTOutputStream_,
    0x831FC7BC, 0x6347, 0x44BC, 0xB4, 0x7B, 0xC0, 0xC6, 0x57, 0xB5, 0xBF, 0x73);

// {CD8E756A-5FC7-4624-984B-2AF02925B91F}
DEFINE_GUID(IID_IMiniportWaveRTInputStream_,
    0xCD8E756A, 0x5FC7, 0x4624, 0x98, 0x4B, 0x2A, 0xF0, 0x29, 0x25, 0xB9, 0x1F);

// {B4C90A32-5791-11D0-86F9-00A0C911B544}
DEFINE_GUID(CLSID_PortTopology,
    0xB4C90A32, 0x5791, 0x11D0, 0x86, 0xF9, 0x00, 0xA0, 0xC9, 0x11, 0xB5, 0x44);

// {B4C90A31-5791-11D0-86F9-00A0C911B544}
DEFINE_GUID(IID_IMiniportTopology_,
    0xB4C90A31, 0x5791, 0x11D0, 0x86, 0xF9, 0x00, 0xA0, 0xC9, 0x11, 0xB5, 0x44);

// {B4C90A30-5791-11D0-86F9-00A0C911B544}
DEFINE_GUID(IID_IPortTopology_,
    0xB4C90A30, 0x5791, 0x11D0, 0x86, 0xF9, 0x00, 0xA0, 0xC9, 0x11, 0xB5, 0x44);

// {B4C90A24-5791-11D0-86F9-00A0C911B544}
DEFINE_GUID(IID_IMiniport_,
    0xB4C90A24, 0x5791, 0x11D0, 0x86, 0xF9, 0x00, 0xA0, 0xC9, 0x11, 0xB5, 0x44);

// {E1CD9915-CAB1-4103-BB2F-7DC09C9BE942}
DEFINE_GUID(IID_IPortClsStreamResourceManager_,
    0xE1CD9915, 0xCAB1, 0x4103, 0xBB, 0x2F, 0x7D, 0xC0, 0x9C, 0x9B, 0xE9, 0x42);

// {0D500BAE-D565-469D-A0E2-F283760D7148}
DEFINE_GUID(IID_IPortClsStreamResourceManager2_,
    0x0D500BAE, 0xD565, 0x469D, 0xA0, 0xE2, 0xF2, 0x83, 0x76, 0x0D, 0x71, 0x48);

// {5DADB7DC-A2CB-4540-A4A8-425EE4AE9051}
DEFINE_GUID(IID_IPinCount_,
    0x5DADB7DC, 0xA2CB, 0x4540, 0xA4, 0xA8, 0x42, 0x5E, 0xE4, 0xAE, 0x90, 0x51);

// {29CC9AB1-E89D-413C-B6B2-F6D50005D063}
DEFINE_GUID(IID_IPinName_,
    0x29CC9AB1, 0xE89D, 0x413C, 0xB6, 0xB2, 0xF6, 0xD5, 0x00, 0x05, 0xD0, 0x63);

// {3DD648B8-969F-11D1-95A9-00C04FB925D3}
DEFINE_GUID(IID_IPowerNotify_,
    0x3DD648B8, 0x969F, 0x11D1, 0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3);

// {706F2368-4086-47F5-B913-57B76EED1A32}
DEFINE_GUID(IID_IAdapterPnpManagement_,
    0x706F2368, 0x4086, 0x47F5, 0xB9, 0x13, 0x57, 0xB7, 0x6E, 0xED, 0x1A, 0x32);

// {793417D0-35FE-11D1-AD08-00A0C90AB1B0}
DEFINE_GUID(IID_IAdapterPowerManagement_,
    0x793417D0, 0x35FE, 0x11D1, 0xAD, 0x08, 0x00, 0xA0, 0xC9, 0x0A, 0xB1, 0xB0);

// {E0F92E5D-67F5-48EE-B57A-7D1E90C5F43D}
DEFINE_GUID(IID_IAdapterPowerManagement2_,
    0xE0F92E5D, 0x67F5, 0x48EE, 0xB5, 0x7A, 0x7D, 0x1E, 0x90, 0xC5, 0xF4, 0x3D);

// {A8C7303E-F80C-4BC9-B2E3-FB2D08BE920F}
DEFINE_GUID(IID_IAdapterPowerManagement3_,
    0xA8C7303E, 0xF80C, 0x4BC9, 0xB2, 0xE3, 0xFB, 0x2D, 0x08, 0xBE, 0x92, 0x0F);

// {2EBF536C-EF57-4C64-BEDC-25C1A6D668E6}
DEFINE_GUID(IID_IMiniportAudioEngineNode_,
    0x2EBF536C, 0xEF57, 0x4C64, 0xBE, 0xDC, 0x25, 0xC1, 0xA6, 0xD6, 0x68, 0xE6);

// {B532678C-BE50-472D-9973-8A6F16594989}
DEFINE_GUID(IID_IMiniportAudioSignalProcessing_,
    0xB532678C, 0xBE50, 0x472D, 0x99, 0x73, 0x8A, 0x6F, 0x16, 0x59, 0x49, 0x89);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ETW PROVIDER
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// {71549463-5E1E-4B7E-9F93-A65606E50D64}
DEFINE_GUID(ETW_PROVIDER_GUID,
    0x71549463, 0x5E1E, 0x4B7E, 0x9F, 0x93, 0xA6, 0x56, 0x06, 0xE5, 0x0D, 0x64);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// KS FORMAT GUIDS (duplicated here for clarity; ksmedia.h has most of these)
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// {B49EEC73-C88F-40E1-8848-D3CE2C4B0051}
DEFINE_GUID(KSPROPSETID_AudioEffectsDiscovery_,
    0xB49EEC73, 0xC88F, 0x40E1, 0x88, 0x48, 0xD3, 0xCE, 0x2C, 0x4B, 0x00, 0x51);

// {C034FDB0-FF4C-4788-B3B6-BF3E15CDC3E9}
DEFINE_GUID(KSPROPSETID_AudioModule_,
    0xC034FDB0, 0xFF4C, 0x4788, 0xB3, 0xB6, 0xBF, 0x3E, 0x15, 0xCD, 0xC3, 0xE9);

// {4509F757-2D46-4637-8E62-CE7DB944F57B}
DEFINE_GUID(KSPROPSETID_Jack_,
    0x4509F757, 0x2D46, 0x4637, 0x8E, 0x62, 0xCE, 0x7D, 0xB9, 0x44, 0xF5, 0x7B);

// {1464EDA5-6A8F-11D1-9AA7-00A0C9223196}
DEFINE_GUID(KSPROPSETID_General_,
    0x1464EDA5, 0x6A8F, 0x11D1, 0x9A, 0xA7, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);

// {45FFAAA0-6E1B-11D0-BCF2-444553540000}
DEFINE_GUID(KSPROPSETID_Audio_,
    0x45FFAAA0, 0x6E1B, 0x11D0, 0xBC, 0xF2, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);

// {185FEDE0-9905-11D1-95A9-00C04FB925D3}
DEFINE_GUID(KSAUDFNAME_MASTER_VOLUME_,
    0x185FEDE0, 0x9905, 0x11D1, 0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3);

// {185FEDE1-9905-11D1-95A9-00C04FB925D3}
DEFINE_GUID(KSAUDFNAME_MASTER_MUTE_,
    0x185FEDE1, 0x9905, 0x11D1, 0x95, 0xA9, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3);

// {97E99BA0-BDEA-11CF-A5D6-28DB04C10000}
DEFINE_GUID(KSPROPTYPESETID_GENERAL_PROP,
    0x97E99BA0, 0xBDEA, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PROPERTY ID CONSTANTS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define KSPROPERTY_JACK_DESCRIPTION_ID      1
#define KSPROPERTY_JACK_DESCRIPTION2_ID     2
#define KSPROPERTY_GENERAL_COMPONENTID_ID   0
#define KSPROPERTY_AUDIO_VOLUMELEVEL_ID     4
#define KSPROPERTY_AUDIO_MUTE_ID            5
#define KSPROPERTY_AUDIOEFFECTSDISCOVERY_EFFECTSLIST_ID 1
#define KSPROPERTY_AUDIOMODULE_DESCRIPTORS_ID           1
#define KSPROPERTY_AUDIOMODULE_COMMAND_ID               2
#define KSPROPERTY_AUDIOMODULE_NOTIFICATION_DEVICE_ID_ID 3

#define VT_I4   3
#define VT_BOOL 11
#define KSPROPERTY_MEMBER_STEPPEDRANGES 2

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PIN INDEX CONSTANTS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define KSPIN_WAVE_SINK     0
#define KSPIN_WAVE_BRIDGE   1
#define KSPIN_TOPO_BRIDGE   0
#define KSPIN_TOPO_LINEOUT  1

// KS constants not always in older headers
#ifndef MM_ALLOCATE_FULLY_REQUIRED
#define MM_ALLOCATE_FULLY_REQUIRED 0x00000004
#endif
