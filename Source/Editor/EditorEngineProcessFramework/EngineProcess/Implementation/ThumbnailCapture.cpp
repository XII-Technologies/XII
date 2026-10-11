/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <Texture/Image/ImageUtils.h>

xiiResult xiiCaptureThumbnail(xiiGALTexture* pTexture, xiiSizeU32 outputSize, xiiImage& out_image)
{
  out_image.Clear();
  if (!pTexture || !outputSize.HasNonZeroArea() || !xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (pTexture->GetDevice().Borrow() != pDevice)
    return XII_FAILURE;

  const auto& source = pTexture->GetDescription();
  if (source.m_Type != xiiGALResourceDimension::Texture2D || source.m_uiSampleCount != 1 ||
      (source.m_Format != xiiGALResourceFormat::RGBA8UNormalized && source.m_Format != xiiGALResourceFormat::RGBA8UNormalizedSRGB))
    return XII_FAILURE;

  auto pQueue       = pDevice->GetCommandQueue();
  auto pCommandList = pDevice->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
  if (!pQueue || !pCommandList)
    return XII_FAILURE;

  xiiGALTextureReadback readback(pDevice);
  pCommandList->Begin();
  readback.Enqueue(pCommandList, {.m_pTexture = pTexture});
  pCommandList->End();
  pQueue->WaitForFenceValue(pQueue->Submit(pCommandList));
  readback.WaitForNextCompleted();
  const auto capture = readback.GetCompleted();
  if (!capture)
    return XII_FAILURE;

  xiiGALTextureCreationDescription description;
  description.m_Type   = xiiGALResourceDimension::Texture2D;
  description.m_Size   = {capture.m_uiWidth, capture.m_uiHeight};
  description.m_Format = pTexture->GetDescription().m_Format;
  xiiImage image;
  image.ResetAndAlloc(description);

  pCommandList->Begin();
  xiiGALTextureMipLevelData      mipLevel;
  xiiGALMappedTextureSubresource mapped;
  xiiResult                      result = pCommandList->MapTextureSubresource(capture.m_pStagingTexture, mipLevel, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped);
  if (result.Succeeded())
  {
    if (mapped.m_pData && mapped.m_uiStride >= image.GetRowPitch())
    {
      for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
      {
        xiiMemoryUtils::Copy(image.GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y),
                             xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt8*>(mapped.m_pData), y * mapped.m_uiStride), static_cast<size_t>(image.GetRowPitch()));
      }
    }
    else
    {
      result = XII_FAILURE;
    }
    if (pCommandList->UnmapTextureSubresource(capture.m_pStagingTexture, mipLevel).Failed())
      result = XII_FAILURE;
  }
  pCommandList->End();
  pQueue->WaitForFenceValue(pQueue->Submit(pCommandList));
  readback.RecycleStagingTexture(capture.m_pStagingTexture);
  XII_SUCCEED_OR_RETURN(result);

  xiiImage scaled;
  XII_SUCCEED_OR_RETURN(xiiImageUtils::Scale(image, scaled, outputSize.width, outputSize.height));
  out_image.ResetAndMove(std::move(scaled));
  return XII_SUCCESS;
}
