// Copyright (c) 2026 Randall Rosas (Slategray). All rights reserved.

#include "leyline_miniport.h"

// Implementation of the standard CUnknown base class.
// PortCls expects this to be present in the driver.

CUnknown::CUnknown(PUNKNOWN pUnknownOuter)
    : m_lRefCount(1) // Start with 1 reference.
{
    // If we are aggregated, use the outer unknown. Otherwise, we are our own outer unknown.
    m_pUnknownOuter = pUnknownOuter ? pUnknownOuter : reinterpret_cast<PUNKNOWN>(static_cast<INonDelegatingUnknown*>(this));
}

CUnknown::~CUnknown()
{
}

STDMETHODIMP_(ULONG) CUnknown::NonDelegatingAddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG) CUnknown::NonDelegatingRelease()
{
    ULONG lRefCount = InterlockedDecrement(&m_lRefCount);
    if (lRefCount == 0)
    {
        delete this;
    }
    return lRefCount;
}

STDMETHODIMP_(NTSTATUS) CUnknown::NonDelegatingQueryInterface(REFIID riid, PVOID* ppvObject)
{
    if (IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObject = reinterpret_cast<PVOID>(static_cast<INonDelegatingUnknown*>(this));
        NonDelegatingAddRef();
        return STATUS_SUCCESS;
    }
    *ppvObject = nullptr;
    return STATUS_NOINTERFACE;
}
