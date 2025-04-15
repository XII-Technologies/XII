#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/CommandEncoder/CommandListD3D11.h>
#include <GraphicsD3D11/CommandEncoder/CommandQueueD3D11.h>
#include <GraphicsD3D11/Device/DeviceD3D11.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandQueueD3D11, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALCommandQueueD3D11::xiiGALCommandQueueD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALCommandQueue(pDeviceD3D11, creationDescription), m_pImmediateContext(pDeviceD3D11->GetImmediateContext()), m_WaitForGPUEventHandle{CreateEvent(nullptr, false, false, nullptr)}
{
}

xiiGALCommandQueueD3D11::~xiiGALCommandQueueD3D11() = default;

void xiiGALCommandQueueD3D11::InitializePlatform()
{
  xiiGALDeviceD3D11* pDeviceD3D11 = static_cast<xiiGALDeviceD3D11*>(m_pDevice);

  D3D11_FENCE_FLAG fenceFlags = D3D11_FENCE_FLAG_NONE;
  XII_HRESULT_TO_ASSERT(pDeviceD3D11->GetD3D11Device()->CreateFence(0U, fenceFlags, IID_PPV_ARGS(&m_pD3D11DFence)));

  m_pImmediateContext->Signal(m_pD3D11DFence, 0U);

  // Allocate a single command list.
  xiiGALCommandListCreationDescription commandListDescription = {.m_QueueType = m_Description.m_QueueType};
  m_pCommandListD3D11                                         = XII_NEW(pDeviceD3D11->GetAllocator(), xiiGALCommandListD3D11, pDeviceD3D11, this, commandListDescription);

  xiiStringBuilder sb;
  sb.SetFormat("Main Command List");
  m_pCommandListD3D11->SetDebugName(sb);
}

void xiiGALCommandQueueD3D11::DeInitializePlatform()
{
  m_pCommandListD3D11.Clear();

  CloseHandle(m_WaitForGPUEventHandle);

  XII_GAL_D3D11_RELEASE(m_pD3D11DFence);
}

xiiUInt64 xiiGALCommandQueueD3D11::GetCompletedFenceValue()
{
  xiiUInt64 uiCompletedFenceValue = m_pD3D11DFence->GetCompletedValue();
  XII_ASSERT_DEV(uiCompletedFenceValue != UINT64_MAX, "If the device has been removed, the return value will be UINT64_MAX");

  xiiUInt64 uiCurrentFenceValue = m_LastCompletedFenceValue.load();
  while (!m_LastCompletedFenceValue.compare_exchange_weak(uiCurrentFenceValue, xiiMath::Max(uiCurrentFenceValue, uiCompletedFenceValue)))
  {
    // If exchange fails, uiCurrentFenceValue will hold the actual value of m_LastCompletedFenceValue
  }
  return m_LastCompletedFenceValue.load();
}

xiiUInt64 xiiGALCommandQueueD3D11::WaitForIdle()
{
  xiiUInt64 uiLastSignaledFenceValue = m_NextFenceValue.fetch_add(1);

  m_pImmediateContext->Signal(m_pD3D11DFence, uiLastSignaledFenceValue);

  if (GetCompletedFenceValue() < uiLastSignaledFenceValue)
  {
    m_pD3D11DFence->SetEventOnCompletion(uiLastSignaledFenceValue, m_WaitForGPUEventHandle);
    WaitForSingleObject(m_WaitForGPUEventHandle, INFINITE);
    XII_ASSERT_ALWAYS(GetCompletedFenceValue() == uiLastSignaledFenceValue, "Unexpected signaled fence value");
  }
  return uiLastSignaledFenceValue;
}

xiiGALCommandList* xiiGALCommandQueueD3D11::BeginCommandList()
{
  XII_ASSERT_DEV(m_pCommandListD3D11->GetRecordingState() == xiiGALCommandList::RecordingState::Reset || m_pCommandListD3D11->GetRecordingState() == xiiGALCommandList::RecordingState::Ended, "There is an active D3D11 command list. D3D11 backend supports only a single active command list.");

  if (m_pCommandListD3D11->GetRecordingState() != xiiGALCommandList::RecordingState::Reset && m_pCommandListD3D11->GetRecordingState() != xiiGALCommandList::RecordingState::Ended)
    return nullptr;

  m_pCommandListD3D11->Begin();

  XII_ASSERT_DEV(m_pCommandListD3D11 != nullptr && m_pCommandListD3D11->GetRecordingState() == xiiGALCommandList::RecordingState::Recording, "The retrieved command list is not begun.");

  return m_pCommandListD3D11.Borrow();
}

xiiUInt64 xiiGALCommandQueueD3D11::SubmitCommandList(xiiGALCommandList* pCommandList)
{
  if (pCommandList == nullptr)
    return GetCompletedFenceValue();

  // Increment the value before submitting the list
  xiiUInt64 uiFenceValue = m_NextFenceValue.fetch_add(1);

  // Signal the fence. This must be done atomically with command list submission.
  m_pImmediateContext->Signal(m_pD3D11DFence, uiFenceValue);

  pCommandList->Reset();

  return uiFenceValue;
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_CommandEncoder_Implementation_CommandQueueD3D11);
