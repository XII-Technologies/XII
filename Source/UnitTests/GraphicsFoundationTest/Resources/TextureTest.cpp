/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/Threading/Thread.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TextureView.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>

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
