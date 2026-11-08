/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/Texture3DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TextureView.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <Texture/Utilities/TextureAssetFile.h>

XII_CREATE_SIMPLE_TEST_GROUP(TextureResidency);

XII_CREATE_SIMPLE_TEST(TextureResidency, DeviceBudget)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;

  auto* pDevice = environment.GetDevice();
  if (pDevice->GetGraphicsDeviceType() != xiiGALGraphicsDeviceType::Vulkan)
    return;

  auto* pBudget = static_cast<xiiCVarInt*>(xiiCVar::FindCVarByName("Streaming.GpuBudgetMiB"));
  auto* pUpload = static_cast<xiiCVarInt*>(xiiCVar::FindCVarByName("Streaming.TextureUploadMiB"));
  if (!XII_TEST_BOOL(pBudget != nullptr && pUpload != nullptr))
    return;
  const xiiInt32 iPreviousBudget = pBudget->GetValue();
  const xiiInt32 iPreviousUpload = pUpload->GetValue();
  XII_SCOPE_EXIT(*pBudget = iPreviousBudget; *pUpload = iPreviousUpload);

  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_BindFlags = xiiGALBindFlags::VertexBuffer;
  bufferDescription.m_uiSize    = 2U * 1024U * 1024U;
  auto pRequiredBuffer          = pDevice->CreateBuffer(bufferDescription);
  if (!XII_TEST_BOOL(pRequiredBuffer != nullptr))
    return;

  auto textureDescription                            = xiiGALTextureUtilities::GetDefaultTexture2DDescription();
  textureDescription.m_Size                          = {4U, 4U};
  textureDescription.m_Format                        = xiiGALResourceFormat::RGBA8UNormalized;
  textureDescription.m_Usage                         = xiiGALResourceUsage::Immutable;
  textureDescription.m_BindFlags                     = xiiGALBindFlags::ShaderResource;
  const xiiUInt8               pixelBytes[4 * 4 * 4] = {};
  xiiGALTextureSubResourceData subResource;
  subResource.m_pData         = xiiConstByteBlobPtr(pixelBytes, sizeof(pixelBytes));
  subResource.m_uiStride      = 16U;
  subResource.m_uiDepthStride = sizeof(pixelBytes);
  xiiGALTextureData textureData(xiiMakeArrayPtr(&subResource, 1U));
  textureData.m_bStreamingRefinement = true;

  *pBudget = 1;
  pDevice->BeginFrame();
  XII_TEST_BOOL(pDevice->IsStreamingMemoryPressured());
  XII_TEST_BOOL(pDevice->CreateTexture(textureDescription, &textureData) == nullptr);

  xiiRenderGraphResourceCache cache(environment.GetDeviceShared());
  cache.BeginFrame(1U, 0U);
  XII_TEST_BOOL(cache.AcquireBuffer(bufferDescription) != nullptr);
  cache.EndFrame();
  cache.BeginFrame(2U, 0U);
  XII_TEST_INT(cache.GetPooledResourceCount(), 0);
  cache.EndFrame();
  cache.BeginFrame(3U, 1U);
  XII_TEST_INT(cache.GetPooledResourceCount(), 1);
  cache.EndFrame();
  cache.BeginFrame(4U, 1U);
  XII_TEST_INT(cache.GetPooledResourceCount(), 0);
  cache.EndFrame();
  pDevice->EndFrame();

  *pBudget = 0;
  *pUpload = 0;
  pDevice->BeginFrame();
  XII_TEST_BOOL(!pDevice->IsStreamingMemoryPressured());
  XII_TEST_BOOL(pDevice->CreateTexture(textureDescription, &textureData) == nullptr);
  textureData.m_bStreamingRefinement = false;
  XII_TEST_BOOL(pDevice->CreateTexture(textureDescription, &textureData) != nullptr);
  pDevice->EndFrame();

  *pUpload                           = 1;
  textureData.m_bStreamingRefinement = true;
  pDevice->BeginFrame();
  XII_TEST_BOOL(pDevice->CreateTexture(textureDescription, &textureData) != nullptr);
  pDevice->EndFrame();
  pDevice->WaitIdle();
}

XII_CREATE_SIMPLE_TEST(TextureResidency, PromoteMipTail)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;

  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  const auto testPromotion = [&environment]<typename Resource>(const char* szPath, xiiGALResourceDimension::Enum type, xiiUInt32 uiLayersOrDepth) {
    auto description                 = xiiGALTextureUtilities::GetDefaultTexture2DDescription();
    description.m_Type               = type;
    description.m_Size               = {64U, 64U};
    description.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
    description.m_uiMipLevels        = 7U;
    description.m_uiArraySizeOrDepth = uiLayersOrDepth;
    xiiImage image;
    image.ResetAndAlloc(description);
    auto pixels = image.GetBlobPtr<xiiUInt8>();
    xiiMemoryUtils::ZeroFill(pixels.GetPtr(), pixels.GetCount());

    const auto sampler = xiiGALGraphicsUtilities::GetDefaultSamplerDescription();
    {
      xiiAssetFileHeader header;
      header.SetFileHashAndVersion(1, 2);
      xiiFileWriter file;
      if (!XII_TEST_RESULT(file.Open(szPath)) || !XII_TEST_RESULT(xiiTextureAssetFile::WriteImage(file, image, sampler, header)))
        return;
    }

    const auto                    handle = xiiResourceManager::LoadResource<Resource>(szPath);
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    writer << xiiGALTextureUtilities::GetDefaultTexture2DDescription();
    writer << sampler;
    writer << true;
    xiiImage* pImage = &image;
    if (!XII_TEST_RESULT(writer.WriteBytes(&pImage, sizeof(pImage))))
      return;
    xiiMemoryStreamReader reader(&storage);
    xiiResourceManager::SetResourceLowResData(handle, &reader);

    xiiResourceLock<Resource> resource(handle, xiiResourceAcquireMode::PointerOnly);
    auto                      pCoarse = resource->GetGALTexture();
    if (!XII_TEST_BOOL(pCoarse != nullptr))
      return;
    XII_TEST_INT(pCoarse->GetDescription().m_uiMipLevels, 6);
    XII_TEST_INT(resource->GetNumQualityLevelsLoadable(), 1);

    xiiResourceManager::PreloadResource(handle);
    const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
    xiiTaskSystem::WaitForCondition([&] { return !xiiResourceManager::IsAnyLoadingInProgress() || xiiTime::Now() > deadline; });
    if (!XII_TEST_BOOL(!xiiResourceManager::IsAnyLoadingInProgress()))
      return;

    auto pFull = resource->GetGALTexture();
    if (!XII_TEST_BOOL(pFull != nullptr && pFull != pCoarse))
      return;
    XII_TEST_INT(pFull->GetDescription().m_uiMipLevels, 7);
    XII_TEST_INT(resource->GetNumQualityLevelsLoadable(), 0);
    XII_TEST_INT(resource->GetMemoryUsage().m_uiMemoryGPU, pFull->GetMemoryConsumption());
    XII_TEST_INT(pCoarse->GetRefCount(), 1);

    auto* pDevice = environment.GetDevice();
    if (pDevice->GetGraphicsDeviceType() == xiiGALGraphicsDeviceType::Vulkan)
    {
      const auto getOccupiedBytes = [pDevice]() {
        xiiHybridArray<xiiGALMemoryHeapStatistics, 16> heaps;
        pDevice->GetMemoryHeapStatistics(heaps);
        xiiUInt64 uiOccupied = 0U;
        for (const auto& heap : heaps)
          uiOccupied += heap.m_uiAllocatedBytes;
        return uiOccupied;
      };

      pDevice->WaitIdle();
      const xiiUInt64 uiBefore      = getOccupiedBytes();
      const xiiUInt64 uiCoarseBytes = pCoarse->GetMemoryConsumption();
      pCoarse.Clear();
      pDevice->WaitIdle();
      const xiiUInt64 uiAfter = getOccupiedBytes();
      XII_TEST_BOOL(uiBefore >= uiAfter && uiBefore - uiAfter == uiCoarseBytes);
      xiiTestFramework::Output(xiiTestOutput::Details, "%s made %llu coarse bytes reusable and retained a %llu-byte replacement.", szPath,
                               static_cast<unsigned long long>(uiCoarseBytes), static_cast<unsigned long long>(pFull->GetMemoryConsumption()));
    }
  };

  testPromotion.operator()<xiiTexture2DResource>(":imgout/Residency/Promotion.xiiBinTexture2D", xiiGALResourceDimension::Texture2D, 1U);
  testPromotion.operator()<xiiTexture3DResource>(":imgout/Residency/Promotion.xiiBinTexture3D", xiiGALResourceDimension::Texture3D, 4U);
  testPromotion.operator()<xiiTextureCubeResource>(":imgout/Residency/Promotion.xiiBinTextureCube", xiiGALResourceDimension::TextureCube, 6U);
}

namespace
{
  class DefaultTextureViewThread : public xiiThread
  {
  public:
    explicit DefaultTextureViewThread(xiiSharedPtr<xiiGALTexture> texture) : xiiThread("Default texture view lifetime"), m_Texture(std::move(texture)) {}
    bool m_bFailed = false;

  private:
    xiiUInt32 Run() override
    {
      for (xiiUInt32 i = 0; i < 1024; ++i)
      {
        auto view = m_Texture->GetDefaultView(xiiGALTextureViewType::ShaderResource);
        if (view == nullptr || view != m_Texture->GetDefaultView(xiiGALTextureViewType::ShaderResource) || view->GetTexture() != m_Texture)
          m_bFailed = true;
      }
      return 0;
    }
    xiiSharedPtr<xiiGALTexture> m_Texture;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(Resources, Texture)
{
  xiiGPUTestingEnvironment environment;
  XII_TEST_BOOL(environment.Initialize().Succeeded());
  const auto initialDeviceReferences = environment.GetDevice()->GetRefCount();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "2D texture and default views")
  {
    xiiGALTextureCreationDescription description;
    description.m_Type               = xiiGALResourceDimension::Texture2D;
    description.m_Size               = xiiSizeU32(32U, 16U);
    description.m_uiArraySizeOrDepth = 1U;
    description.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
    description.m_uiMipLevels        = 3U;
    description.m_uiSampleCount      = 1U;
    description.m_BindFlags          = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget;
    description.m_Usage              = xiiGALResourceUsage::Mutable;

    xiiSharedPtr<xiiGALTexture> pTexture = environment.GetDevice()->CreateTexture(description);
    XII_TEST_BOOL(pTexture != nullptr);
    XII_TEST_INT(pTexture->GetRefCount(), 1); // No default view owns the resource before it is requested.

    XII_TEST_BOOL(pTexture->GetDescription() == description);
    XII_TEST_BOOL(pTexture->GetDevice().Borrow() == environment.GetDevice());
    XII_TEST_BOOL(pTexture->GetMemoryConsumption() >= 32U * 16U * 4U);
    XII_TEST_BOOL(pTexture->GetExternalMemoryKind().IsNoFlagSet());

    xiiSharedPtr<xiiGALTextureView> pSRV = pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);
    xiiSharedPtr<xiiGALTextureView> pRTV = pTexture->GetDefaultView(xiiGALTextureViewType::RenderTarget);
    XII_TEST_BOOL(pSRV != nullptr);
    XII_TEST_BOOL(pRTV != nullptr);
    XII_TEST_BOOL(pSRV->GetTexture() == pTexture);
    XII_TEST_BOOL(pRTV->GetTexture() == pTexture);
    XII_TEST_INT(pSRV->GetDescription().m_uiMipLevelCount, 3U);
    XII_TEST_INT(pRTV->GetDescription().m_uiMipLevelCount, 1U);
    XII_TEST_BOOL(pSRV == pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    XII_TEST_INT(pSRV->GetRefCount(), 1); // The cache itself owns no reference.
    pSRV.Clear();
    XII_TEST_INT(pTexture->GetRefCount(), 2); // The client and the retained render-target view.
    pSRV = pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);
    XII_TEST_BOOL(pSRV != nullptr);
    pTexture.Clear();
    XII_TEST_INT(pSRV->GetTexture()->GetDescription().m_Size.width, 32);
    XII_TEST_BOOL(pSRV->GetTexture() == pRTV->GetTexture());
    pRTV.Clear();
    XII_TEST_INT(pSRV->GetTexture()->GetRefCount(), 2); // The view plus this temporary query.
    pSRV.Clear();
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Concurrent default view acquisition and last release")
  {
    xiiGALTextureCreationDescription description;
    description.m_Type      = xiiGALResourceDimension::Texture2D;
    description.m_Size      = {4, 4};
    description.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    description.m_BindFlags = xiiGALBindFlags::ShaderResource;
    auto texture            = environment.GetDevice()->CreateTexture(description);
    if (!XII_TEST_BOOL(texture != nullptr))
      return;
    // On devices without concurrent creation, retain the initial view so both
    // workers exercise cache acquisition without creating platform objects.
    auto                     retained = environment.GetDevice()->GetFeatures().m_MultithreadedResourceCreation == xiiGALDeviceFeatureState::Enabled ?
                          xiiSharedPtr<xiiGALTextureView>() :
                          texture->GetDefaultView(xiiGALTextureViewType::ShaderResource);
    DefaultTextureViewThread first(texture), second(texture);
    first.Start();
    second.Start();
    first.Join();
    second.Join();
    XII_TEST_BOOL(!first.m_bFailed && !second.m_bFailed);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "External texture import and repeated cross-device binary semaphore handoff")
  {
    auto* producer = environment.GetDevice();
    if (producer->GetFeatures().m_NativeFence == xiiGALDeviceFeatureState::Enabled && producer->GetFeatures().m_ExternalMemory == xiiGALDeviceFeatureState::Enabled && producer->GetFeatures().m_ExternalSemaphore == xiiGALDeviceFeatureState::Enabled)
    {
      xiiGALDeviceCreationDescription deviceDescription;
      deviceDescription.m_GraphicsDeviceType                 = xiiGALGraphicsDeviceType::Vulkan;
      deviceDescription.m_ValidationLevel                    = xiiGALDeviceValidationLevel::Standard;
      deviceDescription.m_DeviceFeatures.m_NativeFence       = xiiGALDeviceFeatureState::Enabled;
      deviceDescription.m_DeviceFeatures.m_ExternalMemory    = xiiGALDeviceFeatureState::Enabled;
      deviceDescription.m_DeviceFeatures.m_ExternalSemaphore = xiiGALDeviceFeatureState::Enabled;
      auto consumer                                          = xiiGALDeviceFactory::CreateDevice("Vulkan", xiiFoundation::GetDefaultAllocator(), deviceDescription);
      if (!XII_TEST_BOOL(consumer != nullptr) || !XII_TEST_RESULT(consumer->Initialize()))
        return;
      xiiGALTextureCreationDescription description;
      description.m_Type                   = xiiGALResourceDimension::Texture2D;
      description.m_Size                   = {16, 8};
      description.m_Format                 = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      description.m_BindFlags              = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
      description.m_ExternalMemory.m_Flags = xiiGALExternalMemoryFlags::BinarySemaphore;
      auto exported                        = producer->CreateTexture(description, nullptr, xiiGALExternalMemoryKind::Exportable);
      if (!XII_TEST_BOOL(exported != nullptr))
        return;
      description.m_ExternalMemory = exported->GetExternalMemoryDescription();
      auto imported                = consumer->CreateTexture(description, nullptr, xiiGALExternalMemoryKind::Imported);
      if (!XII_TEST_BOOL(imported != nullptr))
        return;
      description.m_ExternalMemory           = {};
      auto                  local            = consumer->CreateTexture(description);
      auto                  producerCommands = producer->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer});
      auto                  consumerCommands = consumer->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer});
      auto*                 producerQueue    = producer->GetCommandQueue();
      auto*                 consumerQueue    = consumer->GetCommandQueue();
      xiiGALTextureReadback readback(consumer);
      for (xiiUInt32 frame = 0; frame < 12; ++frame)
      {
        producerCommands->Begin();
        producerCommands->ClearRenderTargetView(exported->GetDefaultView(xiiGALTextureViewType::RenderTarget), frame % 2 ? xiiColor::Blue : xiiColor::Red);
        XII_TEST_RESULT(exported->ReleaseExternalMemory(producerCommands));
        producerCommands->End();
        producerQueue->Submit(producerCommands);
        consumerCommands->Begin();
        XII_TEST_RESULT(imported->AcquireExternalMemory(consumerCommands));
        consumerCommands->CopyTexture(imported, local);
        XII_TEST_RESULT(imported->ReleaseExternalMemory(consumerCommands));
        XII_TEST_RESULT(readback.Enqueue(consumerCommands, {.m_pTexture = local}));
        consumerCommands->End();
        consumerQueue->WaitForFenceValue(consumerQueue->Submit(consumerCommands));
        readback.WaitForNextCompleted();
        const auto capture = readback.GetCompleted();
        if (!XII_TEST_BOOL(static_cast<bool>(capture)))
          return;
        consumerCommands->Begin();
        xiiGALMappedTextureSubresource mapped;
        XII_TEST_RESULT(consumerCommands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped));
        if (XII_TEST_BOOL(mapped.m_pData != nullptr))
        {
          const auto* pixel = static_cast<const xiiUInt8*>(mapped.m_pData);
          XII_TEST_INT(pixel[0], frame % 2 ? 0 : 255);
          XII_TEST_INT(pixel[1], 0);
          XII_TEST_INT(pixel[2], frame % 2 ? 255 : 0);
          XII_TEST_INT(pixel[3], 255);
          XII_TEST_RESULT(consumerCommands->UnmapTextureSubresource(capture.m_pStagingTexture, {}));
        }
        consumerCommands->End();
        consumerQueue->WaitForFenceValue(consumerQueue->Submit(consumerCommands));
        readback.RecycleStagingTexture(capture.m_pStagingTexture);
        producerCommands->Begin();
        XII_TEST_RESULT(exported->AcquireExternalMemory(producerCommands));
        producerCommands->End();
        producerQueue->WaitForFenceValue(producerQueue->Submit(producerCommands));
      }
      consumer->WaitIdle();
    }
  }
  environment.GetDevice()->WaitIdle();
  XII_TEST_INT(environment.GetDevice()->GetRefCount(), initialDeviceReferences);
}
