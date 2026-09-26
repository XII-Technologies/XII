/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>

XII_CREATE_SIMPLE_TEST(Tools, TextureReadback)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Region copy, metadata, mapped pixels, and pool reuse")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      constexpr xiiUInt32 width  = 8U;
      constexpr xiiUInt32 height = 6U;
      xiiUInt8            pixels[width * height * 4U];
      for (xiiUInt32 y = 0; y < height; ++y)
      {
        for (xiiUInt32 x = 0; x < width; ++x)
        {
          const xiiUInt32 offset = (y * width + x) * 4U;
          pixels[offset + 0U]    = static_cast<xiiUInt8>(x * 13U);
          pixels[offset + 1U]    = static_cast<xiiUInt8>(y * 17U);
          pixels[offset + 2U]    = static_cast<xiiUInt8>(x + y * 3U);
          pixels[offset + 3U]    = 255U;
        }
      }

      xiiGALTextureCreationDescription textureDescription;
      textureDescription.m_Type               = xiiGALResourceDimension::Texture2D;
      textureDescription.m_Size               = xiiSizeU32(width, height);
      textureDescription.m_uiArraySizeOrDepth = 1U;
      textureDescription.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
      textureDescription.m_uiMipLevels        = 1U;
      textureDescription.m_uiSampleCount      = 1U;
      textureDescription.m_BindFlags          = xiiGALBindFlags::ShaderResource;
      textureDescription.m_Usage              = xiiGALResourceUsage::Mutable;
      xiiSharedPtr<xiiGALTexture> pTexture    = environment.GetDevice()->CreateTexture(textureDescription);
      XII_TEST_BOOL(pTexture != nullptr);
      if (pTexture == nullptr)
        continue;

      xiiGALCommandListCreationDescription commandListDescription;
      commandListDescription.m_QueueFlags          = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer;
      xiiSharedPtr<xiiGALCommandList> pCommandList = environment.GetDevice()->CreateCommandList(commandListDescription);
      XII_TEST_BOOL(pCommandList != nullptr);
      if (pCommandList == nullptr)
        continue;

      xiiGALTextureReadback readback(environment.GetDevice());
      XII_TEST_BOOL(!readback.HasCompleted());
      XII_TEST_BOOL(!static_cast<bool>(readback.GetCompleted()));

      xiiGALTextureMipLevelData          mipLevel;
      const xiiBoundingBoxU32            fullBox = xiiBoundingBoxU32::MakeFromMinMax(xiiVec3U32::MakeZero(), xiiVec3U32(width, height, 1U));
      const xiiGALTextureSubResourceData sourceData(xiiConstByteBlobPtr(pixels), width * 4U, width * height * 4U);

      xiiGALTextureReadback::ReadbackRequest request;
      request.m_pTexture    = pTexture.Borrow();
      request.m_uiTextureID = 91U;
      request.m_uiRegionX   = 2U;
      request.m_uiRegionY   = 1U;
      request.m_uiRegionW   = 4U;
      request.m_uiRegionH   = 3U;

      pCommandList->Begin();
      pCommandList->UpdateTexture(pTexture.Borrow(), mipLevel, fullBox, sourceData);
      readback.Enqueue(pCommandList.Borrow(), request);
      pCommandList->End();

      xiiGALCommandQueue* pQueue = environment.GetDevice()->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
      XII_TEST_BOOL(pQueue != nullptr);
      if (pQueue == nullptr)
        continue;

      const xiiUInt64 uiFenceValue = pQueue->Submit(pCommandList.Borrow());
      pQueue->WaitForFenceValue(uiFenceValue);
      readback.WaitForNextCompleted();
      XII_TEST_BOOL(readback.HasCompleted());

      xiiGALTextureReadback::ReadbackCapture capture = readback.GetCompleted();
      XII_TEST_BOOL(static_cast<bool>(capture));
      XII_TEST_INT(capture.m_uiTextureID, 91U);
      XII_TEST_INT(capture.m_uiWidth, 4U);
      XII_TEST_INT(capture.m_uiHeight, 3U);
      XII_TEST_INT(capture.m_uiBytesPerPixel, 4U);
      XII_TEST_INT(capture.m_uiRowStrideBytes, 16U);
      XII_TEST_INT(capture.m_uiRegionX, 2U);
      XII_TEST_INT(capture.m_uiRegionY, 1U);
      XII_TEST_INT(capture.m_uiRegionW, 4U);
      XII_TEST_INT(capture.m_uiRegionH, 3U);

      xiiSharedPtr<xiiGALCommandList> pMapCommandList = environment.GetDevice()->CreateCommandList(commandListDescription);
      pMapCommandList->Begin();
      xiiGALMappedTextureSubresource mapped;
      XII_TEST_BOOL(pMapCommandList->MapTextureSubresource(capture.m_pStagingTexture, mipLevel, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped).Succeeded());
      XII_TEST_BOOL(mapped.m_pData != nullptr);
      if (mapped.m_pData != nullptr)
      {
        for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
        {
          const xiiUInt8* pActual   = xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt8*>(mapped.m_pData), y * mapped.m_uiStride);
          const xiiUInt8* pExpected = &pixels[((request.m_uiRegionY + y) * width + request.m_uiRegionX) * 4U];
          XII_TEST_BOOL(xiiMemoryUtils::IsEqual(pActual, pExpected, capture.m_uiRowStrideBytes));
        }
      }
      XII_TEST_BOOL(pMapCommandList->UnmapTextureSubresource(capture.m_pStagingTexture, mipLevel).Succeeded());
      pMapCommandList->End();

      xiiGALTexture* pFirstStagingTexture = capture.m_pStagingTexture;
      readback.RecycleStagingTexture(capture.m_pStagingTexture);

      pCommandList->Begin();
      readback.Enqueue(pCommandList.Borrow(), request);
      pCommandList->End();
      const xiiUInt64 uiSecondFenceValue = pQueue->Submit(pCommandList.Borrow());
      pQueue->WaitForFenceValue(uiSecondFenceValue);
      readback.WaitForNextCompleted();
      capture = readback.GetCompleted();
      XII_TEST_BOOL(capture.m_pStagingTexture == pFirstStagingTexture);
      readback.RecycleStagingTexture(capture.m_pStagingTexture);
    }
  }
}
