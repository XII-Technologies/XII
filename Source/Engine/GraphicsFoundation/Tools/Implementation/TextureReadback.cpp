#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/Threading/Mutex.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/ScopedDebugGroup.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

xiiGALTextureReadback::xiiGALTextureReadback(xiiSharedPtr<xiiGALDevice> pDevice) :
  m_pDevice(std::move(pDevice))
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "Invalid device provided.");

  xiiGALFenceCreationDescription fenceDescription;
  fenceDescription.m_Type = xiiGALFenceType::CpuWaitOnly;
  m_pFence                = m_pDevice->CreateFence(fenceDescription);
  m_pFence->SetDebugName("Texture Readback Fence");
}

xiiGALTextureReadback::~xiiGALTextureReadback() = default;

xiiSharedPtr<xiiGALTexture> xiiGALTextureReadback::AcquireStagingTexture(const xiiGALTextureCreationDescription& description)
{
  xiiSharedPtr<xiiGALTexture> pStagingTexture;

  // Try pool first
  {
    XII_LOCK(m_PoolMutex);

    // Find last matching to reduce moves; pool is small
    for (xiiUInt32 i = m_StagingPool.GetCount(); i > 0; --i)
    {
      xiiSharedPtr<xiiGALTexture>& pStagingCandidate = m_StagingPool[i - 1];

      if (pStagingCandidate->GetDescription() == description)
      {
        pStagingTexture = std::move(pStagingCandidate);
        m_StagingPool.RemoveAtAndSwap(i - 1);
        break;
      }
    }
  }

  if (!pStagingTexture)
  {
    pStagingTexture = m_pDevice->CreateTexture(description);

    pStagingTexture->SetDebugName("Texture Readback Staging");
  }

  return pStagingTexture;
}

void xiiGALTextureReadback::Enqueue(xiiSharedPtr<xiiGALCommandList> pCommandList, const ReadbackRequest& request)
{
  XII_ASSERT_DEV(pCommandList != nullptr, "Invalid command list.");
  XII_ASSERT_DEV(request, "Invalid readback request.");

  const xiiGALTextureCreationDescription& srcDesc = request.m_pTexture->GetDescription();

  // Resolve subresource dimensions
  const xiiSizeU32 fullSize    = srcDesc.m_Size;
  xiiUInt32        uiMipWidth  = xiiMath::Max(1U, fullSize.width >> request.m_uiMipLevel);
  xiiUInt32        uiMipHeight = xiiMath::Max(1U, fullSize.height >> request.m_uiMipLevel);

  // Region box.
  xiiBoundingBoxU32 box;
  if (request.m_uiRegionW > 0U && request.m_uiRegionH > 0U)
  {
    box = xiiBoundingBoxU32::MakeFromMinMax({request.m_uiRegionX, request.m_uiRegionY, 0U}, {request.m_uiRegionX + request.m_uiRegionW, request.m_uiRegionY + request.m_uiRegionH, 1U});
  }
  else
  {
    box = xiiBoundingBoxU32::MakeFromMinMax(xiiVec3U32::MakeZero(), {uiMipWidth, uiMipHeight, 1U});
  }

  // Destination staging texture description.
  xiiGALTextureCreationDescription stagingDescription;
  stagingDescription.m_Type               = xiiGALResourceDimension::Texture2D;
  stagingDescription.m_Size               = {box.m_vMax.x - box.m_vMin.x, box.m_vMax.y - box.m_vMin.y};
  stagingDescription.m_uiArraySizeOrDepth = 1U;
  stagingDescription.m_Format             = srcDesc.m_Format;
  stagingDescription.m_uiMipLevels        = 1U;
  stagingDescription.m_uiSampleCount      = 1U;
  stagingDescription.m_BindFlags          = xiiGALBindFlags::None;
  stagingDescription.m_Usage              = xiiGALResourceUsage::Staging;
  stagingDescription.m_CPUAccessFlags     = xiiGALCPUAccessFlag::Read;

  xiiSharedPtr<xiiGALTexture> pStagingTexture = AcquireStagingTexture(stagingDescription);

  xiiGALTextureMipLevelData sourceMipLevelData;
  sourceMipLevelData.m_uiMipLevel   = request.m_uiMipLevel;
  sourceMipLevelData.m_uiArraySlice = request.m_uiArraySlice;

  xiiGALTextureMipLevelData destinationMipLevelData;
  destinationMipLevelData.m_uiMipLevel   = 0U;
  destinationMipLevelData.m_uiArraySlice = 0U;

  {
    xiiGALScopedDebugGroup group(pCommandList, "Texture Readback");

    pCommandList->CopyTextureRegion(request.m_pTexture, sourceMipLevelData, box, pStagingTexture, destinationMipLevelData, xiiVec3U32::MakeZero());

    const xiiUInt64 uiFenceValue = m_uiNextFenceValue++;
    pCommandList->EnqueueSignal(m_pFence, uiFenceValue);

    Pending pendingCapture;
    pendingCapture.m_pStagingTexture = std::move(pStagingTexture);
    pendingCapture.m_uiFenceValue    = uiFenceValue;

    // Fill capture metadata
    pendingCapture.m_CaptureMeta.m_pStagingTexture    = pendingCapture.m_pStagingTexture;
    pendingCapture.m_CaptureMeta.m_uiTextureID        = request.m_uiTextureID;
    pendingCapture.m_CaptureMeta.m_uiMipLevel         = request.m_uiMipLevel;
    pendingCapture.m_CaptureMeta.m_uiArraySlice       = request.m_uiArraySlice;
    pendingCapture.m_CaptureMeta.m_uiRegionX          = box.m_vMin.x;
    pendingCapture.m_CaptureMeta.m_uiRegionY          = box.m_vMin.y;
    pendingCapture.m_CaptureMeta.m_uiRegionW          = box.m_vMax.x - box.m_vMin.x;
    pendingCapture.m_CaptureMeta.m_uiRegionH          = box.m_vMax.y - box.m_vMin.y;
    pendingCapture.m_CaptureMeta.m_bRepackTightRows   = request.m_bRepackTightRows;
    pendingCapture.m_CaptureMeta.m_bConvertBGRAtoRGBA = request.m_bConvertBGRAtoRGBA;

    const xiiGALTextureCreationDescription& destinationDescription = pendingCapture.m_pStagingTexture->GetDescription();
    xiiGALResourceFormatDescription         formatProperties       = xiiGALTextureUtilities::GetResourceFormatProperties(destinationDescription.m_Format);

    pendingCapture.m_CaptureMeta.m_uiWidth          = destinationDescription.m_Size.width;
    pendingCapture.m_CaptureMeta.m_uiHeight         = destinationDescription.m_Size.height;
    pendingCapture.m_CaptureMeta.m_uiBytesPerPixel  = formatProperties.m_uiComponentSize * formatProperties.m_uiComponentCount;
    pendingCapture.m_CaptureMeta.m_uiRowStrideBytes = pendingCapture.m_CaptureMeta.m_uiWidth * pendingCapture.m_CaptureMeta.m_uiBytesPerPixel;

    XII_LOCK(m_PendingMutex);
    m_Pending.PushBack(std::move(pendingCapture));
  }
}

bool xiiGALTextureReadback::HasCompleted() const
{
  XII_LOCK(m_PendingMutex);

  if (m_Pending.IsEmpty())
    return false;

  const auto& oldestPendingCapture = m_Pending.PeekFront();
  return m_pFence->GetCompletedValue() >= oldestPendingCapture.m_uiFenceValue;
}

xiiGALTextureReadback::ReadbackCapture xiiGALTextureReadback::GetCompleted()
{
  ReadbackCapture capture;
  XII_LOCK(m_PendingMutex);

  if (!m_Pending.IsEmpty())
  {
    auto& oldestPendingCapture = m_Pending.PeekFront();

    if (m_pFence->GetCompletedValue() >= oldestPendingCapture.m_uiFenceValue)
    {
      capture = oldestPendingCapture.m_CaptureMeta;
      m_Pending.PopFront();
    }
  }
  return capture;
}

void xiiGALTextureReadback::WaitForNextCompleted()
{
  XII_LOCK(m_PendingMutex);

  if (!m_Pending.IsEmpty())
  {
    const xiiUInt64 uiFenceValue = m_Pending.PeekFront().m_uiFenceValue;
    m_pFence->Wait(uiFenceValue);
  }
}

void xiiGALTextureReadback::RecycleStagingTexture(xiiSharedPtr<xiiGALTexture>&& pStagingTexture)
{
  if (!pStagingTexture)
    return;

  XII_LOCK(m_PoolMutex);
  m_StagingPool.PushBack(std::move(pStagingTexture));
}

void xiiGALTextureReadback::RepackTightRows(const void* pSource, void* pDestination, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiBytesPerPixel, xiiUInt32 uiSourceRowStride)
{
  const xiiUInt32 uiTightStride = uiWidth * uiBytesPerPixel;

  for (xiiUInt32 y = 0; y < uiHeight; ++y)
  {
    const xiiUInt8* pSourceRow      = static_cast<const xiiUInt8*>(pSource) + y * uiSourceRowStride;
    xiiUInt8*       pDestinationRow = static_cast<xiiUInt8*>(pDestination) + y * uiTightStride;

    memcpy(pDestinationRow, pSourceRow, uiTightStride);
  }
}

void xiiGALTextureReadback::ConvertBGRAtoRGBA(const void* pSource, void* pDestination, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiSourceRowStride)
{
  const xiiUInt32 uiTightStride = uiWidth * 4U;

  for (xiiUInt32 uiY = 0; uiY < uiHeight; ++uiY)
  {
    const xiiUInt8* pSourceRow      = static_cast<const xiiUInt8*>(pSource) + uiY * uiSourceRowStride;
    xiiUInt8*       pDestinationRow = static_cast<xiiUInt8*>(pDestination) + uiY * uiTightStride;

    for (xiiUInt32 uiX = 0; uiX < uiWidth; ++uiX)
    {
      const xiiUInt8 uiB = pSourceRow[4 * uiX + 0];
      const xiiUInt8 uiG = pSourceRow[4 * uiX + 1];
      const xiiUInt8 uiR = pSourceRow[4 * uiX + 2];
      const xiiUInt8 uiA = pSourceRow[4 * uiX + 3];

      pDestinationRow[4 * uiX + 0] = uiR;
      pDestinationRow[4 * uiX + 1] = uiG;
      pDestinationRow[4 * uiX + 2] = uiB;
      pDestinationRow[4 * uiX + 3] = uiA;
    }
  }
}
