#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/FenceD3D12.h>

xiiGALFenceD3D12::xiiGALFenceD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALFenceCreationDescription& creationDescription) :
  xiiGALFence(pDeviceD3D12, creationDescription), m_pFenceCompleteEvent{CreateEvent(NULL, TRUE, FALSE, NULL)}
{
}

xiiGALFenceD3D12::~xiiGALFenceD3D12() = default;

xiiResult xiiGALFenceD3D12::InitPlatform()
{
  xiiGALDeviceD3D12* pDeviceD3D12 = static_cast<xiiGALDeviceD3D12*>(m_pDevice);

  if (m_pFenceCompleteEvent == NULL)
  {
    xiiLog::Error("Failed to create fence complete event.");
    return XII_FAILURE;
  }

  D3D12_FENCE_FLAGS fenceFlags = D3D12_FENCE_FLAG_SHARED;
  XII_HRESULT_TO_FAILURE_LOG(pDeviceD3D12->GetD3D12Device()->CreateFence(0U, fenceFlags, IID_PPV_ARGS(&m_pD3D12Fence)));

  return XII_SUCCESS;
}

xiiResult xiiGALFenceD3D12::DeInitPlatform()
{
  // Schedule deletion on device.
  XII_ASSERT_NOT_IMPLEMENTED;

  if (m_pFenceCompleteEvent != NULL && m_pFenceCompleteEvent != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_pFenceCompleteEvent);
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
      xiiThreadUtils::Sleep(xiiTime::MakeFromMicroseconds(1U));
    }
  }
}

void xiiGALFenceD3D12::SetDebugNamePlatform(xiiStringView sName)
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
