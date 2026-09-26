/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/DynamicBuffer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALDynamicBuffer, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALDynamicBuffer::xiiGALDynamicBuffer(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALBufferCreationDescription& description) :
  xiiGALObject(), m_pDevice(pDevice), m_Description(description), m_uiPendingSize(description.m_uiSize)
{
  /// \todo Support sparse buffer.

  // Tracks the current internal buffer size, starts at 0.
  m_Description.m_uiSize = 0U;

  if (m_pDevice && m_uiPendingSize > 0)
  {
    XII_LOCK(m_Mutex);

    InitializeBuffer();
  }
}

xiiGALDynamicBuffer::~xiiGALDynamicBuffer() = default;

xiiGALBufferCreationDescription xiiGALDynamicBuffer::GetDescription() const
{
  XII_LOCK(m_Mutex);

  return m_Description;
}

xiiSharedPtr<xiiGALBuffer> xiiGALDynamicBuffer::GetBuffer() const
{
  XII_LOCK(m_Mutex);

  return m_pBuffer;
}

bool xiiGALDynamicBuffer::PendingUpdate() const
{
  XII_LOCK(m_Mutex);

  return m_uiPendingSize != m_Description.m_uiSize;
}

xiiSharedPtr<xiiGALBuffer> xiiGALDynamicBuffer::Resize(xiiSharedPtr<xiiGALCommandList> pCommandList, xiiUInt64 uiNewSize, bool bDiscardContent)
{
  {
    XII_LOCK(m_Mutex);

    if (m_Description.m_uiSize != uiNewSize)
    {
      m_uiPendingSize = uiNewSize;

      if (m_Description.m_Usage != xiiGALResourceUsage::Sparse)
      {
        if (!m_pStaleBuffer)
        {
          m_pStaleBuffer = std::move(m_pBuffer);
        }
        else
        {
          XII_ASSERT_DEV(!m_pBuffer || uiNewSize == 0, "There is a non-null stale buffer. This likely indicates that Resize() has been called multiple times with different sizes, but copy has not been committed by providing a non-null command list to either Resize() or Update().");
        }

        if (m_uiPendingSize == 0)
        {
          m_pStaleBuffer.Clear();
          m_pBuffer.Clear();

          m_Description.m_uiSize = 0U;
        }

        if (bDiscardContent)
        {
          m_pStaleBuffer.Clear();
        }
      }
    }
  }

  // Resolve pending resize.
  {
    XII_LOCK(m_Mutex);

    ResolvePendingResize(pCommandList, true);

    return m_pBuffer;
  }
}

xiiSharedPtr<xiiGALBuffer> xiiGALDynamicBuffer::Update(xiiSharedPtr<xiiGALCommandList> pCommandList)
{
  {
    XII_LOCK(m_Mutex);

    ResolvePendingResize(pCommandList, false);
  }

  // Fence wait logic: we need the command list to wait on the fence if required.
  {
    XII_LOCK(m_Mutex);

    if (m_uiLastAfterResizeFenceValue + 1 < m_uiNextAfterResizeFenceValue)
    {
      XII_ASSERT_DEV(pCommandList != nullptr, "The command list is invalid, but waiting for the fence is required.");
      XII_ASSERT_DEV(m_pAfterResizeFence != nullptr, "The after resize fence is invalid.");

      m_uiLastAfterResizeFenceValue = m_uiNextAfterResizeFenceValue - 1;

      // Release lock while waiting to avoid blocking other callers that might need to update state.
    }
  }

  // Wait outside the lock to avoid deadlocks and allow other threads to make progress.
  if (m_uiLastAfterResizeFenceValue != 0 && pCommandList != nullptr && m_pAfterResizeFence != nullptr)
  {
    pCommandList->DeviceWaitForFence(m_pAfterResizeFence, m_uiLastAfterResizeFenceValue);
  }

  XII_LOCK(m_Mutex);

  return m_pBuffer;
}

void xiiGALDynamicBuffer::InitializeBuffer()
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "Device is invalid.");

  if (m_Description.m_Usage == xiiGALResourceUsage::Mutable && m_uiPendingSize > 0U)
  {
    xiiGALBufferCreationDescription description = m_Description;
    description.m_uiSize                        = m_uiPendingSize;

    XII_ASSERT_DEV(description.m_Usage != xiiGALResourceUsage::Sparse, "Sparse buffers are not yet supported by xiiGALDynamicBuffer.");

    m_pBuffer = m_pDevice->CreateBuffer(description);

    if (!m_pStaleBuffer)
    {
      // The array was previously empty or its contents were explicitly discarded.
      // There is no deferred copy, so the resize is complete as soon as the new buffer exists.
      m_Description.m_uiSize = m_uiPendingSize;
    }
  }

  XII_ASSERT_DEV(m_pBuffer != nullptr, "Failed to create buffer for a dynamic buffer.");

  m_Version.PostIncrement();
}

void xiiGALDynamicBuffer::ResolvePendingResize(xiiSharedPtr<xiiGALCommandList> pCommandList, bool bPermitNull)
{
  XII_IGNORE_UNUSED(bPermitNull);

  if (!m_pBuffer && m_uiPendingSize > 0)
  {
    if (m_pDevice != nullptr)
    {
      InitializeBuffer();
    }
    else
    {
      XII_ASSERT_DEV(bPermitNull, "Dynamic buffer must be initialized with a valid device to resolve pending resize.");
    }
  }

  if (m_pBuffer && m_Description.m_uiSize != m_uiPendingSize)
  {
    if (pCommandList != nullptr)
    {
      if (m_Description.m_Usage != xiiGALResourceUsage::Sparse)
      {
        CopyStaleBuffer(pCommandList);
      }

      m_Description.m_uiSize = m_uiPendingSize;
    }
    else
    {
      XII_ASSERT_DEV(bPermitNull, "Dynamic buffer must be initialized with a valid command list to resolve pending resize.");
    }
  }
}

void xiiGALDynamicBuffer::CopyStaleBuffer(xiiSharedPtr<xiiGALCommandList> pCommandList)
{
  if (!m_pStaleBuffer)
    return;

  XII_ASSERT_DEV(m_pBuffer != nullptr, "The internal buffer is invalid.");

  xiiUInt64 uiCopySize = xiiMath::Min(m_pBuffer->GetDescription().m_uiSize, m_pStaleBuffer->GetDescription().m_uiSize);

  pCommandList->CopyBufferRegion(m_pStaleBuffer, 0, m_pBuffer, 0, uiCopySize);

  m_pStaleBuffer.Clear();
}
