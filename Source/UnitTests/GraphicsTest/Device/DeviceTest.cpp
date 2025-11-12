#include <GraphicsTest/GraphicsTestPCH.h>

#include <GraphicsTest/TestingEnvironment/TestingEnvironment.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <TestFramework/Framework/TestFramework.h>

// Include resource and state descriptions used in tests
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/States/RasterizerState.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

XII_CREATE_SIMPLE_TEST_GROUP(Device);

XII_CREATE_SIMPLE_TEST(Device, Basics)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create and initialize device")
  {
    xiiGALDeviceCreationDescription deviceCreationDescription;
    deviceCreationDescription.m_DeviceFeatures.m_WireframeFill = xiiGALDeviceFeatureState::Optional;

    xiiStringView sGraphicsAPIName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, "Vulkan");

    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDeviceFactory::CreateDevice(sGraphicsAPIName, xiiFoundation::GetDefaultAllocator(), deviceCreationDescription);
    XII_TEST_BOOL(pDevice != nullptr);
    if (pDevice)
    {
      XII_TEST_RESULT(pDevice->Initialize());
      pDevice.Clear();
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create window and swapchain via testing environment")
  {
    xiiUniquePtr<xiiGPUTestingEnvironmentVulkan> pEnvironment = XII_DEFAULT_NEW(xiiGPUTestingEnvironmentVulkan);

    XII_TEST_RESULT(pEnvironment->Initialize());

    XII_TEST_RESULT(pEnvironment->CreateWindow(128, 128));

    XII_TEST_RESULT(pEnvironment->CreateSwapChainForWindow(128, 128));

    XII_TEST_BOOL(pEnvironment->GetSwapChain() != nullptr);
    XII_TEST_BOOL(pEnvironment->GetDepthStencilTexture() != nullptr);

    // Basic frame operations
    pEnvironment->BeginFrame();
    pEnvironment->EndFrame();
    pEnvironment->Present();

    pEnvironment->DestroySwapChain();
    pEnvironment->DestroyWindow();
    pEnvironment->Shutdown();

    pEnvironment.Clear();
  }
}

XII_CREATE_SIMPLE_TEST(Device, ResourceCreation)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create common resource types and validate parameters")
  {
    xiiUniquePtr<xiiGPUTestingEnvironmentVulkan> pEnvironment = XII_DEFAULT_NEW(xiiGPUTestingEnvironmentVulkan);
    XII_TEST_RESULT(pEnvironment->Initialize());
    XII_TEST_RESULT(pEnvironment->CreateWindow(16, 16));
    XII_TEST_RESULT(pEnvironment->CreateSwapChainForWindow(16, 16));

    xiiGALDevice* pDevice = pEnvironment->GetDevice();
    XII_TEST_BOOL(pDevice != nullptr);
    if (!pDevice)
    {
      pEnvironment->DestroySwapChain();
      pEnvironment->DestroyWindow();
      pEnvironment->Shutdown();
      return;
    }

    // 1) Create a structured shader resource buffer (valid)
    {
      xiiGALBufferCreationDescription desc;
      desc.m_uiSize = 256;
      desc.m_BindFlags = xiiGALBindFlags::ShaderResource;
      desc.m_Mode = xiiGALBufferMode::Structured;
      desc.m_uiElementByteStride = 16;
      desc.m_Usage = xiiGALResourceUsage::Mutable;
      desc.m_CPUAccessFlags = xiiGALCPUAccessFlag::None;

      xiiSharedPtr<xiiGALBuffer> pBuffer = pDevice->CreateBuffer(desc);
      XII_TEST_BOOL(pBuffer != nullptr);
    }

    // 2) Create a formatted buffer (valid) using a known format element size
    {
      const xiiEnum<xiiGALResourceFormat> format = xiiGALResourceFormat::RGBA8UNormalized;
      const xiiGALResourceFormatDescription& fmtDesc = xiiGALTextureUtilities::GetResourceFormatProperties(format);

      xiiGALBufferCreationDescription desc;
      desc.m_uiSize = 1024;
      desc.m_BindFlags = xiiGALBindFlags::ShaderResource;
      desc.m_Mode = xiiGALBufferMode::Formatted;
      desc.m_uiElementByteStride = fmtDesc.GetElementSize();
      desc.m_Usage = xiiGALResourceUsage::Mutable;

      xiiSharedPtr<xiiGALBuffer> pBuffer = pDevice->CreateBuffer(desc);
      XII_TEST_BOOL(pBuffer != nullptr);
    }

    // 3) Create a simple 2D texture (valid)
    {
      xiiGALTextureCreationDescription texDesc;
      texDesc.m_Type = xiiGALResourceDimension::Texture2D;
      texDesc.m_Size = xiiSizeU32(32, 32);
      texDesc.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
      texDesc.m_uiMipLevels = 1;
      texDesc.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget;
      texDesc.m_Usage = xiiGALResourceUsage::Mutable;

      xiiSharedPtr<xiiGALTexture> pTex = pDevice->CreateTexture(texDesc);
      XII_TEST_BOOL(pTex != nullptr);
    }

    // 4) Create a sampler (valid)
    {
      xiiGALSamplerCreationDescription sampDesc;
      sampDesc.m_MinFilter = xiiGALFilterType::Linear;
      sampDesc.m_MagFilter = xiiGALFilterType::Linear;
      sampDesc.m_MipFilter = xiiGALFilterType::Linear;

      xiiSharedPtr<xiiGALSampler> pSamp = pDevice->CreateSampler(sampDesc);
      XII_TEST_BOOL(pSamp != nullptr);
    }

    // 5) Create default blend state (no blending enabled): should succeed
    {
      xiiGALBlendStateCreationDescription blendDesc;
      xiiSharedPtr<xiiGALBlendState> pBlend = pDevice->CreateBlendState(blendDesc);
      XII_TEST_BOOL(pBlend != nullptr);
    }

    // 6) Create a rasterizer state (valid)
    {
      xiiGALRasterizerStateCreationDescription rastDesc;
      rastDesc.m_FillMode = xiiGALFillMode::Solid;
      rastDesc.m_CullMode = xiiGALCullMode::Back;

      xiiSharedPtr<xiiGALRasterizerState> pRast = pDevice->CreateRasterizerState(rastDesc);
      XII_TEST_BOOL(pRast != nullptr);
    }

    // Cleanup
    pEnvironment->DestroySwapChain();
    pEnvironment->DestroyWindow();
    pEnvironment->Shutdown();
    pEnvironment.Clear();
  }
}

XII_CREATE_SIMPLE_TEST(Device, ViewsAndDefaultViews)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Default views are created for textures with appropriate bind flags")
  {
    xiiUniquePtr<xiiGPUTestingEnvironmentVulkan> pEnvironment = XII_DEFAULT_NEW(xiiGPUTestingEnvironmentVulkan);
    XII_TEST_RESULT(pEnvironment->Initialize());
    XII_TEST_RESULT(pEnvironment->CreateWindow(32, 32));
    XII_TEST_RESULT(pEnvironment->CreateSwapChainForWindow(32, 32));

    xiiGALDevice* pDevice = pEnvironment->GetDevice();
    XII_TEST_BOOL(pDevice != nullptr);
    if (!pDevice)
    {
      pEnvironment->DestroySwapChain();
      pEnvironment->DestroyWindow();
      pEnvironment->Shutdown();
      return;
    }

    // Create a texture with shader resource and render target bind flags and ensure default views exist
    xiiGALTextureCreationDescription texDesc;
    texDesc.m_Type = xiiGALResourceDimension::Texture2D;
    texDesc.m_Size = xiiSizeU32(16, 16);
    texDesc.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
    texDesc.m_uiMipLevels = 1;
    texDesc.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget;
    texDesc.m_Usage = xiiGALResourceUsage::Mutable;

    xiiSharedPtr<xiiGALTexture> pTex = pDevice->CreateTexture(texDesc);
    XII_TEST_BOOL(pTex != nullptr);

    if (pTex)
    {
      // Shader resource default view should exist
      xiiSharedPtr<xiiGALTextureView> pSRV = pTex->GetDefaultView(xiiGALTextureViewType::ShaderResource);
      XII_TEST_BOOL(pSRV != nullptr);

      // Render target default view should exist if render target bind flag was accepted
      xiiSharedPtr<xiiGALTextureView> pRTV = pTex->GetDefaultView(xiiGALTextureViewType::RenderTarget);
      XII_TEST_BOOL(pRTV != nullptr);
    }

    // Cleanup
    pEnvironment->DestroySwapChain();
    pEnvironment->DestroyWindow();
    pEnvironment->Shutdown();
    pEnvironment.Clear();
  }
}

XII_CREATE_SIMPLE_TEST(Device, DefaultDeviceAndQueues)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Set and retrieve default device, query queues")
  {
    xiiGALDeviceCreationDescription deviceCreationDescription;
    deviceCreationDescription.m_DeviceFeatures.m_WireframeFill = xiiGALDeviceFeatureState::Optional;

    xiiStringView sGraphicsAPIName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, "Vulkan");

    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDeviceFactory::CreateDevice(sGraphicsAPIName, xiiFoundation::GetDefaultAllocator(), deviceCreationDescription);
    XII_TEST_BOOL(pDevice != nullptr);
    if (!pDevice)
      return;

    XII_TEST_RESULT(pDevice->Initialize());

    // Set default device
    xiiGALDevice::SetDefaultDevice(pDevice);
    XII_TEST_BOOL(xiiGALDevice::HasDefaultDevice());
    xiiSharedPtr<xiiGALDevice> pDefault = xiiGALDevice::GetDefaultDevice();
    XII_TEST_BOOL(pDefault.Borrow() == pDevice.Borrow());

    // Query command queue
    xiiGALCommandQueue* pQueue = pDevice->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
    XII_TEST_BOOL(pQueue != nullptr);

    // Clean up default device
    xiiGALDevice::SetDefaultDevice(xiiSharedPtr<xiiGALDevice>());
    XII_TEST_BOOL(!xiiGALDevice::HasDefaultDevice());

    pDevice.Clear();
  }
}
