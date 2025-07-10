#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/DynamicBuffer.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALDynamicBuffer, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALDynamicBuffer::xiiGALDynamicBuffer(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALBufferCreationDescription& description) :
  xiiGALObject(), m_pDevice(pDevice), m_Description(description), m_uiPendingSize(description.m_uiSize)
{
  /// \todo Support sparse buffer.

  m_Description.m_uiSize = 0U; // Current buffer size.

  if (m_pDevice && m_uiPendingSize > 0)
  {
    InitializeBuffer();
  }
}

xiiGALDynamicBuffer::~xiiGALDynamicBuffer() = default;

xiiSharedPtr<xiiGALBuffer> xiiGALDynamicBuffer::Resize(xiiSharedPtr<xiiGALCommandList> pCommandList, xiiUInt64 uiNewSize, bool bDiscardContent)
{
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

  ResolvePendingResize(pCommandList, true);

  return m_pBuffer ? m_pBuffer : nullptr;
}

xiiSharedPtr<xiiGALBuffer> xiiGALDynamicBuffer::Update(xiiSharedPtr<xiiGALCommandList> pCommandList)
{
  ResolvePendingResize(pCommandList, false);

  if (m_uiLastAfterResizeFenceValue + 1 < m_uiNextAfterResizeFenceValue)
  {
    XII_ASSERT_DEV(pCommandList != nullptr, "The command list is invalid, but waiting for the fence is required.");
    XII_ASSERT_DEV(m_pAfterResizeFence != nullptr, "The after resize fence is invalid.");

    m_uiLastAfterResizeFenceValue = m_uiNextAfterResizeFenceValue - 1;

    pCommandList->DeviceWaitForFence(m_pAfterResizeFence, m_uiLastAfterResizeFenceValue);
  }

  return m_pBuffer ? m_pBuffer : nullptr;
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

    if (m_Description.m_uiSize == 0U)
    {
      // The array was previously empty, nothing to copy.
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
        ResizeDefaultBuffer(pCommandList);
      }

      m_Description.m_uiSize = m_uiPendingSize;
    }
    else
    {
      XII_ASSERT_DEV(bPermitNull, "Dynamic buffer must be initialized with a valid command list to resolve pending resize.");
    }
  }
}

void xiiGALDynamicBuffer::ResizeDefaultBuffer(xiiSharedPtr<xiiGALCommandList> pCommandList)
{
  if (!m_pStaleBuffer)
    return;

  XII_ASSERT_DEV(m_pBuffer != nullptr, "The internal buffer is invalid.");

  xiiUInt64 uiCopySize = xiiMath::Min(m_pBuffer->GetDescription().m_uiSize, m_pStaleBuffer->GetDescription().m_uiSize);

  pCommandList->CopyBufferRegion(m_pStaleBuffer, 0, m_pBuffer, 0, uiCopySize);

  m_pStaleBuffer.Clear();
}
