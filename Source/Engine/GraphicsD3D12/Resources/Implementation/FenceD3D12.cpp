/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Pools/FencePoolD3D12.h>
#include <GraphicsD3D12/Resources/FenceD3D12.h>

xiiGALFenceD3D12::xiiGALFenceD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALFenceCreationDescription& creationDescription) :
  xiiGALFence(std::move(pDeviceD3D12), creationDescription), m_pFenceCompleteEvent{CreateEvent(NULL, TRUE, FALSE, NULL)}
{
}

xiiGALFenceD3D12::~xiiGALFenceD3D12()
{
  if (m_pD3D12Fence != nullptr)
  {
    xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();
    if (m_bFromFencePool && pDeviceD3D12 != nullptr && pDeviceD3D12->GetD3D12FencePool() != nullptr)
    {
      // Reset pooled fence value to a predictable baseline before reusing.
      m_pD3D12Fence->Signal(0U);
      pDeviceD3D12->GetD3D12FencePool()->ReclaimFence(m_pD3D12Fence);
    }
    else if (pDeviceD3D12 != nullptr)
    {
      IUnknown* pObject = m_pD3D12Fence;
      pDeviceD3D12->SafeReleaseDeviceObject(pObject);
    }
    else
    {
      XII_GAL_D3D12_RELEASE(m_pD3D12Fence);
    }

    m_pD3D12Fence = nullptr;
    m_bFromFencePool = false;
  }

  if (m_pFenceCompleteEvent != NULL && m_pFenceCompleteEvent != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_pFenceCompleteEvent);
  }
}

xiiResult xiiGALFenceD3D12::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12 = m_pDevice.Downcast<xiiGALDeviceD3D12>();

  if (m_pFenceCompleteEvent == NULL)
  {
    xiiLog::Error("Failed to create fence complete event.");
    return XII_FAILURE;
  }

  xiiGALFencePoolD3D12* pFencePoolD3D12 = pDeviceD3D12->GetD3D12FencePool();
  if (pFencePoolD3D12 == nullptr)
  {
    xiiLog::Error("Failed to create D3D12 fence '{}': fence pool is unavailable.", GetDebugName());
    return XII_FAILURE;
  }

  m_pD3D12Fence = pFencePoolD3D12->RequestFence();
  if (m_pD3D12Fence == nullptr)
  {
    xiiLog::Error("Failed to create D3D12 fence '{}': fence pool did not provide a fence.", GetDebugName());
    return XII_FAILURE;
  }

  m_bFromFencePool = true;
  const HRESULT hResult = m_pD3D12Fence->Signal(0U);
  if (FAILED(hResult))
  {
    xiiLog::Warning("Failed to reset pooled D3D12 fence '{}' to zero during initialization: {}.", GetDebugName(), xiiHRESULTtoString(hResult));
  }

  return XII_SUCCESS;
}

xiiUInt64 xiiGALFenceD3D12::GetCompletedValue()
{
  xiiUInt64 uiValue = m_pD3D12Fence->GetCompletedValue();
  XII_ASSERT_DEV(uiValue != xiiMath::MaxValue<xiiUInt64>(), "If the device has been removed, the return value will be UINT64_MAX.");
  return uiValue;
}

void xiiGALFenceD3D12::Signal(xiiUInt64 uiValue)
{
  XII_ASSERT_DEV(m_Description.m_Type == xiiGALFenceType::General, "The fence must be created with xiiGALFenceType::General.");
  XII_ASSERT_DEV(m_pDevice->GetDescription().m_DeviceFeatures.m_NativeFence == xiiGALDeviceFeatureState::Enabled, "CPU fence signal requires the device Native Fence feature.");

  m_pD3D12Fence->Signal(uiValue);
}

void xiiGALFenceD3D12::Wait(xiiUInt64 uiValue)
{
  if (GetCompletedValue() >= uiValue)
    return;

  if (m_pFenceCompleteEvent != NULL)
  {
    m_pD3D12Fence->SetEventOnCompletion(uiValue, m_pFenceCompleteEvent);

    WaitForSingleObject(m_pFenceCompleteEvent, INFINITE);
  }
  else
  {
    while (GetCompletedValue() < uiValue)
    {
      xiiThreadUtils::YieldTimeSlice();
    }
  }
}

void xiiGALFenceD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  if (m_pD3D12Fence != nullptr)
  {
    xiiStringBuilder sb;
    if (FAILED(m_pD3D12Fence->SetPrivateData(WKPDID_D3DDebugObjectName, sName.GetElementCount(), sName.GetData(sb))))
    {
      xiiLog::Error("Failed to set the Direct3D12 fence debug name.");
    }
  }
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_FenceD3D12);
