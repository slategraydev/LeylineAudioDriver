// Copyright (c) 2026 Randall Rosas (Slategray). All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LEYLINE KS DESCRIPTOR TABLES
// Statically-initialized descriptors for Wave and Topology filters.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include "leyline_common.h"
#include "leyline_guids.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// JACK DESCRIPTION STRUCTURES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#if !defined(KSJACK_DESCRIPTION) || !defined(KSJACK_DESCRIPTION_DEFINED)
#define KSJACK_DESCRIPTION_DEFINED
typedef struct _KSJACK_DESCRIPTION
{
    DWORD ChannelMapping;
    DWORD Color;
    DWORD ConnectionType;
    DWORD GeoLocation;
    DWORD GenLocation;
    DWORD PortConnection;
    BOOL  IsConnected;
} KSJACK_DESCRIPTION;
#endif

typedef struct _KSJACK_DESCRIPTION2
{
    DWORD DeviceStateInfo;
    DWORD JackCapabilities;
} KSJACK_DESCRIPTION2;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// COMPONENT ID STRUCTURE  (matches SDK KSCOMPONENTID)
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef struct _KSCOMPONENTID
{
    GUID Manufacturer;
    GUID Product;
    GUID Component;
    GUID Name;
    ULONG Version;
    ULONG Revision;
} KSCOMPONENTID;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PROPERTY DESCRIPTION STRUCTURES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef struct _KSPROPERTY_DESCRIPTION
{
    ULONG AccessFlags;
    ULONG DescriptionSize;
    KSIDENTIFIER PropTypeSet;
    ULONG MembersListCount;
    ULONG Reserved;
} KSPROPERTY_DESCRIPTION_EX;

typedef struct _KSPROPERTY_MEMBERSHEADER
{
    ULONG MembersFlags;
    ULONG MembersSize;
    ULONG MembersCount;
    ULONG Flags;
} KSPROPERTY_MEMBERSHEADER_EX;

typedef struct _KSPROPERTY_STEPPING_LONG
{
    ULONG SteppingDelta;
    ULONG Reserved;
    LONG  SignedMinimum;
    LONG  SignedMaximum;
} KSPROPERTY_STEPPING_LONG_EX;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DATA RANGE AUDIO STRUCTURE
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

typedef struct _KSDATARANGE_AUDIO_EX : public KSDATARANGE
{
    ULONG MaximumChannels;
    ULONG MinimumBitsPerSample;
    ULONG MaximumBitsPerSample;
    ULONG MinimumSampleFrequency;
    ULONG MaximumSampleFrequency;
} KSDATARANGE_AUDIO_EX;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PROPERTY HANDLERS (Forward Declarations)
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NTSTATUS ComponentIdHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS JackDescriptionHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS VolumeHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS MuteHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS PinCategoryHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS PinNameHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS ProposedFormatHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS AudioEffectsDiscoveryHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS AudioModuleHandler(PPCPROPERTY_REQUEST PropertyRequest);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DESCRIPTOR TABLE DECLARATIONS
// Definitions are in descriptors.cpp; extern refs for other translation units.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern const KSDATARANGE_AUDIO_EX   g_PcmDataRange;
extern const KSDATARANGE_AUDIO_EX   g_FloatDataRange;
extern const KSDATARANGE            g_BridgeDataRange;
extern const KSDATARANGE * const    g_WaveDataRanges[2];
extern const KSDATARANGE * const    g_BridgeDataRanges[1];

extern const PCFILTER_DESCRIPTOR    g_WaveRenderFilterDescriptor;
extern const PCFILTER_DESCRIPTOR    g_WaveCaptureFilterDescriptor;
extern const PCFILTER_DESCRIPTOR    g_TopoRenderFilterDescriptor;
extern const PCFILTER_DESCRIPTOR    g_TopoCaptureFilterDescriptor;
