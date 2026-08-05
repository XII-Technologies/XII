/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/CommandEncoder/CommandListD3D12.h>
#include <GraphicsD3D12/CommandEncoder/CommandQueueD3D12.h>
#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/FenceD3D12.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandQueueD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALCommandQueueD3D12::xiiGALCommandQueueD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALCommandQueueCreationDescription& creationDescription, const xiiGALQueueInformationD3D12& queueInformation) :
  xiiGALCommandQueue(pDeviceD3D12, creationDescription), m_QueueInformation(queueInformation)
{
  m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

  if (m_hFenceEvent == nullptr)
  {
    xiiLog::Error("Failed to create D3D12 command queue fence event handle.");
  }

  if (FAILED(pDeviceD3D12->GetD3D12Device()->CreateFence(0U, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pD3D12QueueFence))))
  {
    xiiLog::Error("Failed to create D3D12 command queue fence.");
  }
}

xiiGALCommandQueueD3D12::~xiiGALCommandQueueD3D12()
{
  WaitForIdle();

  XII_GAL_D3D12_RELEASE(m_pD3D12QueueFence);
  XII_GAL_D3D12_RELEASE(m_QueueInformation.m_pCommandQueue);

  if (m_hFenceEvent != nullptr && m_hFenceEvent != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hFenceEvent);

    m_hFenceEvent = nullptr;
  }
}

xiiUInt64 xiiGALCommandQueueD3D12::GetCompletedFenceValue()
{
  XII_ASSERT_DEV(m_pD3D12QueueFence != nullptr, "D3D12 command queue fence is not initialized.");
  XII_ASSERT_DEV(m_hFenceEvent != nullptr && m_hFenceEvent != INVALID_HANDLE_VALUE, "D3D12 command queue fence event handle is invalid.");

  const xiiUInt64 uiValue = m_pD3D12QueueFence->GetCompletedValue();

  if (uiValue == xiiMath::MaxValue<xiiUInt64>())
  {
    xiiLog::Error("D3D12 command queue fence returned UINT64_MAX. The device may have been removed.");

    return uiValue;
  }

  return uiValue;
}

xiiUInt64 xiiGALCommandQueueD3D12::SubmitPlatform(xiiGALCommandList* pCommandList)
{
  XII_ASSERT_DEV(m_pD3D12QueueFence != nullptr, "D3D12 command queue fence is not initialized.");
  XII_ASSERT_DEV(m_hFenceEvent != nullptr && m_hFenceEvent != INVALID_HANDLE_VALUE, "D3D12 command queue fence event handle is invalid.");

  xiiGALCommandListD3D12*  pCommandListD3D12 = xiiDynamicCast<xiiGALCommandListD3D12*>(pCommandList);
  ID3D12CommandList* const pD3D12CommandList = pCommandListD3D12->GetD3D12CommandList();

  for (const auto& waitInfo : pCommandListD3D12->m_WaitFences)
  {
    XII_ASSERT_DEV(waitInfo.m_pFenceD3D12 != nullptr, "Invalid wait-fence entry on command list '{}'.", pCommandListD3D12->GetDebugName());

    const HRESULT hWaitResult = m_QueueInformation.m_pCommandQueue->Wait(waitInfo.m_pFenceD3D12->GetD3D12Fence(), waitInfo.m_uiWaitValue);

    if (FAILED(hWaitResult))
    {
      xiiLog::Error("Failed to enqueue D3D12 queue wait for fence '{}' (value {}) during submit: {}.", waitInfo.m_pFenceD3D12->GetDebugName(), waitInfo.m_uiWaitValue, xiiHRESULTtoString(hWaitResult));

      return m_uiLastSyncPointValue;
    }
  }

  m_QueueInformation.m_pCommandQueue->ExecuteCommandLists(1U, &pD3D12CommandList);

  const xiiUInt64 uiFenceValue = m_uiNextFenceValue.PostIncrement();
  const HRESULT   hResult      = m_QueueInformation.m_pCommandQueue->Signal(m_pD3D12QueueFence, uiFenceValue);

  if (FAILED(hResult))
  {
    xiiLog::Error("Failed to signal D3D12 command queue fence during submit: {}.", xiiHRESULTtoString(hResult));

    return m_uiLastSyncPointValue;
  }

  for (const auto& signalInfo : pCommandListD3D12->m_SignalFences)
  {
    XII_ASSERT_DEV(signalInfo.m_pFenceD3D12 != nullptr, "Invalid signal-fence entry on command list '{}'.", pCommandListD3D12->GetDebugName());

    signalInfo.m_pFenceD3D12->ValidateFenceSignal(signalInfo.m_uiWaitValue);

    const HRESULT hSignalResult = m_QueueInformation.m_pCommandQueue->Signal(signalInfo.m_pFenceD3D12->GetD3D12Fence(), signalInfo.m_uiWaitValue);

    if (FAILED(hSignalResult))
    {
      xiiLog::Error("Failed to enqueue D3D12 queue signal for fence '{}' (value {}) during submit: {}.", signalInfo.m_pFenceD3D12->GetDebugName(), signalInfo.m_uiWaitValue, xiiHRESULTtoString(hSignalResult));

      return m_uiLastSyncPointValue;
    }
  }

  m_uiLastSyncPointValue                     = uiFenceValue;
  pCommandListD3D12->m_uiSubmittedFenceValue = uiFenceValue;

  return uiFenceValue;
}

xiiUInt64 xiiGALCommandQueueD3D12::WaitForIdle()
{
  XII_ASSERT_DEV(m_pD3D12QueueFence != nullptr, "D3D12 command queue fence is not initialized.");
  XII_ASSERT_DEV(m_hFenceEvent != nullptr && m_hFenceEvent != INVALID_HANDLE_VALUE, "D3D12 command queue fence event handle is invalid.");

  const xiiUInt64 uiFenceValue = m_uiNextFenceValue.PostIncrement();
  HRESULT         hResult      = m_QueueInformation.m_pCommandQueue->Signal(m_pD3D12QueueFence, uiFenceValue);

  if (FAILED(hResult))
  {
    xiiLog::Error("Failed to signal D3D12 queue fence for WaitForIdle: {}.", xiiHRESULTtoString(hResult));

    return m_uiLastSyncPointValue;
  }

  m_uiLastSyncPointValue = uiFenceValue;

  if (GetCompletedFenceValue() < uiFenceValue)
  {
    if (m_hFenceEvent != nullptr && m_hFenceEvent != INVALID_HANDLE_VALUE)
    {
      hResult = m_pD3D12QueueFence->SetEventOnCompletion(uiFenceValue, m_hFenceEvent);

      if (SUCCEEDED(hResult))
      {
        WaitForSingleObject(m_hFenceEvent, INFINITE);
      }
      else
      {
        xiiLog::Error("Failed to set D3D12 queue fence completion event: {}.", xiiHRESULTtoString(hResult));
      }
    }

    while (GetCompletedFenceValue() < uiFenceValue)
    {
      xiiThreadUtils::YieldTimeSlice();
    }
  }

  return uiFenceValue;
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_CommandEncoder_Implementation_CommandQueueD3D12);
