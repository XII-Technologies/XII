/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Resources/Buffer.h>

XII_CREATE_SIMPLE_TEST(Resources, BindlessResource)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Generation-checked fence-aware allocation")
  {
    xiiGALBindlessResourceAllocator allocator;
    allocator.Initialize(2U);
    XII_TEST_INT(allocator.GetCapacity(), 2U);
    XII_TEST_INT(allocator.GetAllocatedCount(), 0U);

    const xiiGALBindlessResourceHandle first  = allocator.Allocate();
    const xiiGALBindlessResourceHandle second = allocator.Allocate();
    XII_TEST_BOOL(first.IsValid());
    XII_TEST_BOOL(second.IsValid());
    XII_TEST_BOOL(first.m_uiIndex != second.m_uiIndex);
    XII_TEST_BOOL(allocator.IsAlive(first));
    XII_TEST_BOOL(allocator.IsAlive(second));
    XII_TEST_INT(allocator.GetAllocatedCount(), 2U);
    XII_TEST_BOOL(!allocator.Allocate().IsValid());

    XII_TEST_BOOL(allocator.Retire(first, 7U));
    XII_TEST_BOOL(!allocator.IsAlive(first));
    XII_TEST_BOOL(!allocator.Retire(first, 7U));
    XII_TEST_INT(allocator.GetAllocatedCount(), 1U);
    XII_TEST_BOOL(!allocator.Allocate().IsValid());

    allocator.Collect(6U);
    XII_TEST_BOOL(!allocator.Allocate().IsValid());
    allocator.Collect(7U);
    const xiiGALBindlessResourceHandle recycled = allocator.Allocate();
    XII_TEST_BOOL(recycled.IsValid());
    XII_TEST_INT(recycled.m_uiIndex, first.m_uiIndex);
    XII_TEST_BOOL(recycled.m_uiGeneration != first.m_uiGeneration);
    XII_TEST_BOOL(allocator.IsAlive(recycled));
    XII_TEST_BOOL(!allocator.IsAlive(first));

    allocator.Clear();
    XII_TEST_INT(allocator.GetCapacity(), 0U);
    XII_TEST_INT(allocator.GetAllocatedCount(), 0U);
    XII_TEST_BOOL(!allocator.Allocate().IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Typed table registration, update, retire, collect, and reuse")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALBufferCreationDescription bufferDescription;
      bufferDescription.m_uiSize              = 64U;
      bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
      bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
      bufferDescription.m_uiElementByteStride = 4U;
      xiiSharedPtr<xiiGALBuffer> pBuffer = environment.GetDevice()->CreateBuffer(bufferDescription);
      XII_TEST_BOOL(pBuffer != nullptr);

      xiiGALTextureCreationDescription textureDescription;
      textureDescription.m_Type               = xiiGALResourceDimension::Texture2D;
      textureDescription.m_Size               = xiiSizeU32(4U, 4U);
      textureDescription.m_uiArraySizeOrDepth = 1U;
      textureDescription.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
      textureDescription.m_uiMipLevels        = 1U;
      textureDescription.m_BindFlags          = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      textureDescription.m_Usage              = xiiGALResourceUsage::Mutable;
      xiiSharedPtr<xiiGALTexture> pTexture = environment.GetDevice()->CreateTexture(textureDescription);
      XII_TEST_BOOL(pTexture != nullptr);

      xiiSharedPtr<xiiGALSampler> pSampler = environment.GetDevice()->CreateSampler(xiiGALSamplerCreationDescription());
      XII_TEST_BOOL(pSampler != nullptr);
      if (pBuffer == nullptr || pTexture == nullptr || pSampler == nullptr)
        continue;

      xiiSharedPtr<xiiGALBufferView> pBufferSRV = pBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource);
      xiiSharedPtr<xiiGALBufferView> pBufferUAV = pBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess);
      xiiSharedPtr<xiiGALTextureView> pTextureSRV = pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);
      xiiSharedPtr<xiiGALTextureView> pTextureUAV = pTexture->GetDefaultView(xiiGALTextureViewType::UnorderedAccess);
      XII_TEST_BOOL(pBufferSRV != nullptr && pBufferUAV != nullptr && pTextureSRV != nullptr && pTextureUAV != nullptr);

      xiiGALBindlessResourceTableDescription tableDescription;
      tableDescription.m_uiBufferSRVCapacity  = 2U;
      tableDescription.m_uiBufferUAVCapacity  = 2U;
      tableDescription.m_uiTextureSRVCapacity = 2U;
      tableDescription.m_uiTextureUAVCapacity = 2U;
      tableDescription.m_uiSamplerCapacity    = 2U;

      xiiGALBindlessResourceTable table;
      table.Initialize(tableDescription);
      XII_TEST_BOOL(!table.RegisterBufferSRV(nullptr).IsValid());

      const auto bufferSRV  = table.RegisterBufferSRV(pBufferSRV);
      const auto bufferUAV  = table.RegisterBufferUAV(pBufferUAV);
      const auto textureSRV = table.RegisterTextureSRV(pTextureSRV);
      const auto textureUAV = table.RegisterTextureUAV(pTextureUAV);
      const auto sampler    = table.RegisterSampler(pSampler);
      XII_TEST_BOOL(bufferSRV.IsValid() && bufferUAV.IsValid() && textureSRV.IsValid() && textureUAV.IsValid() && sampler.IsValid());

      const xiiGALBindlessResourceTableStats stats = table.GetStats();
      XII_TEST_INT(stats.m_uiBufferSRVCount, 1U);
      XII_TEST_INT(stats.m_uiBufferUAVCount, 1U);
      XII_TEST_INT(stats.m_uiTextureSRVCount, 1U);
      XII_TEST_INT(stats.m_uiTextureUAVCount, 1U);
      XII_TEST_INT(stats.m_uiSamplerCount, 1U);

      XII_TEST_BOOL(table.UpdateBufferSRV(bufferSRV, pBufferSRV));
      XII_TEST_BOOL(table.UpdateBufferUAV(bufferUAV, pBufferUAV));
      XII_TEST_BOOL(table.UpdateTextureSRV(textureSRV, pTextureSRV));
      XII_TEST_BOOL(table.UpdateTextureUAV(textureUAV, pTextureUAV));
      XII_TEST_BOOL(table.UpdateSampler(sampler, pSampler));
      XII_TEST_BOOL(!table.UpdateSampler({}, pSampler));

      XII_TEST_BOOL(table.RetireTextureSRV(textureSRV, 11U));
      XII_TEST_BOOL(!table.RetireTextureSRV(textureSRV, 11U));
      XII_TEST_INT(table.GetStats().m_uiTextureSRVCount, 0U);
      const auto secondTexture = table.RegisterTextureSRV(pTextureSRV);
      XII_TEST_BOOL(secondTexture.IsValid());
      XII_TEST_BOOL(secondTexture.m_uiIndex != textureSRV.m_uiIndex);
      XII_TEST_BOOL(!table.RegisterTextureSRV(pTextureSRV).IsValid());

      table.Collect(10U);
      XII_TEST_BOOL(!table.RegisterTextureSRV(pTextureSRV).IsValid());
      table.Collect(11U);
      const auto recycledTexture = table.RegisterTextureSRV(pTextureSRV);
      XII_TEST_BOOL(recycledTexture.IsValid());
      XII_TEST_INT(recycledTexture.m_uiIndex, textureSRV.m_uiIndex);
      XII_TEST_BOOL(recycledTexture.m_uiGeneration != textureSRV.m_uiGeneration);

      table.Clear();
      const auto clearedStats = table.GetStats();
      XII_TEST_INT(clearedStats.m_uiBufferSRVCount + clearedStats.m_uiBufferUAVCount + clearedStats.m_uiTextureSRVCount + clearedStats.m_uiTextureUAVCount + clearedStats.m_uiSamplerCount, 0U);
    }
  }
}
