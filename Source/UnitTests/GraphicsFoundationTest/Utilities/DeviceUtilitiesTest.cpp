/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

XII_CREATE_SIMPLE_TEST(Utilities, DeviceUtilities)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Vendor IDs and render-target descriptions")
  {
    XII_TEST_BOOL(xiiGALDeviceUtilities::GetVendorFromID(0x1002U) == xiiGALGraphicsAdapterVendor::AMD);
    XII_TEST_BOOL(xiiGALDeviceUtilities::GetVendorFromID(0x10DEU) == xiiGALGraphicsAdapterVendor::Nvidia);
    XII_TEST_BOOL(xiiGALDeviceUtilities::GetVendorFromID(0x8086U) == xiiGALGraphicsAdapterVendor::Intel);
    XII_TEST_BOOL(xiiGALDeviceUtilities::GetVendorFromID(0x13B5U) == xiiGALGraphicsAdapterVendor::ARM);
    XII_TEST_BOOL(xiiGALDeviceUtilities::GetVendorFromID(0x5143U) == xiiGALGraphicsAdapterVendor::Qualcomm);
    XII_TEST_BOOL(xiiGALDeviceUtilities::GetVendorFromID(0x1414U) == xiiGALGraphicsAdapterVendor::Microsoft);
    XII_TEST_BOOL(xiiGALDeviceUtilities::GetVendorFromID(0x106BU) == xiiGALGraphicsAdapterVendor::Apple);
    XII_TEST_BOOL(xiiGALDeviceUtilities::GetVendorFromID(0U) == xiiGALGraphicsAdapterVendor::Unknown);

    const xiiGALTextureCreationDescription color = xiiGALDeviceUtilities::CreateRenderTargetDescription(xiiSizeU32(128U, 64U), xiiGALResourceFormat::RGBA8UNormalized, 4U);
    XII_TEST_BOOL(color.m_Type == xiiGALResourceDimension::Texture2D);
    XII_TEST_BOOL(color.m_Size == xiiSizeU32(128U, 64U));
    XII_TEST_BOOL(color.m_BindFlags.AreAllSet(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget));
    XII_TEST_BOOL(!color.m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil));
    XII_TEST_INT(color.m_uiSampleCount, 4U);

    const xiiGALTextureCreationDescription depth = xiiGALDeviceUtilities::CreateRenderTargetDescription(xiiSizeU32(32U, 32U), xiiGALResourceFormat::D32Float);
    XII_TEST_BOOL(depth.m_BindFlags.AreAllSet(xiiGALBindFlags::ShaderResource | xiiGALBindFlags::DepthStencil));
    XII_TEST_BOOL(!depth.m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Buffer construction and mapped updates")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiUInt8 vertexData[36];
      for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(vertexData); ++i)
        vertexData[i] = static_cast<xiiUInt8>(i + 1U);

      xiiSharedPtr<xiiGALBuffer> pImmutableVertices = xiiGALDeviceUtilities::CreateVertexBuffer(environment.GetDevice(), 12U, 3U, xiiMakeArrayPtr(vertexData));
      XII_TEST_BOOL(pImmutableVertices != nullptr);
      XII_TEST_BOOL(pImmutableVertices->GetDescription().m_Usage == xiiGALResourceUsage::Immutable);
      XII_TEST_BOOL(pImmutableVertices->GetDescription().m_BindFlags == xiiGALBindFlags::VertexBuffer);
      XII_TEST_INT(pImmutableVertices->GetDescription().m_uiElementByteStride, 12U);
      XII_TEST_INT(pImmutableVertices->GetDescription().m_uiSize, 36U);

      xiiSharedPtr<xiiGALBuffer> pDynamicVertices = xiiGALDeviceUtilities::CreateVertexBuffer(environment.GetDevice(), 12U, 3U);
      XII_TEST_BOOL(pDynamicVertices != nullptr);
      XII_TEST_BOOL(pDynamicVertices->GetDescription().m_Usage == xiiGALResourceUsage::Dynamic);
      XII_TEST_BOOL(pDynamicVertices->GetDescription().m_CPUAccessFlags == xiiGALCPUAccessFlag::Write);

      xiiUInt8 indexData[6] = {0U, 0U, 1U, 0U, 2U, 0U};
      xiiSharedPtr<xiiGALBuffer> pIndices = xiiGALDeviceUtilities::CreateIndexBuffer(environment.GetDevice(), xiiGALDeviceUtilities::IndexType::UShort, 3U, xiiMakeArrayPtr(indexData));
      XII_TEST_BOOL(pIndices != nullptr);
      XII_TEST_BOOL(pIndices->GetDescription().m_BindFlags == xiiGALBindFlags::IndexBuffer);
      XII_TEST_INT(pIndices->GetDescription().m_uiElementByteStride, 2U);
      XII_TEST_INT(pIndices->GetDescription().m_uiSize, 6U);

      xiiSharedPtr<xiiGALBuffer> pConstants = xiiGALDeviceUtilities::CreateConstantBuffer(environment.GetDevice(), 256U, "Unit Test Constants");
      XII_TEST_BOOL(pConstants != nullptr);
      XII_TEST_BOOL(pConstants->GetDescription().m_BindFlags == xiiGALBindFlags::UniformBuffer);
      XII_TEST_STRING(pConstants->GetDebugName(), "Unit Test Constants");

      xiiSharedPtr<xiiGALBuffer> pStaging = xiiGALDeviceUtilities::CreateStagingBuffer(environment.GetDevice(), 64U, "Unit Test Staging");
      XII_TEST_BOOL(pStaging != nullptr);
      if (pStaging != nullptr)
      {
        XII_TEST_BOOL(pStaging->GetDescription().m_BindFlags.IsNoFlagSet());
        XII_TEST_BOOL(pStaging->GetDescription().m_Usage == xiiGALResourceUsage::Staging);
        XII_TEST_BOOL(pStaging->GetDescription().m_CPUAccessFlags == xiiGALCPUAccessFlag::Write);
        XII_TEST_STRING(pStaging->GetDebugName(), "Unit Test Staging");
      }

      xiiGALCommandListCreationDescription commandListDescription;
      commandListDescription.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer;
      xiiSharedPtr<xiiGALCommandList> pCommandList = environment.GetDevice()->CreateCommandList(commandListDescription);
      XII_TEST_BOOL(pCommandList != nullptr);
      if (pCommandList == nullptr || pDynamicVertices == nullptr)
        continue;

      xiiUInt8 replacement[12];
      for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(replacement); ++i)
        replacement[i] = static_cast<xiiUInt8>(0xF0U + i);

      pCommandList->Begin();
      XII_TEST_BOOL(xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList.Borrow(), pDynamicVertices, 12U, xiiMakeArrayPtr(replacement)).Succeeded());
      pCommandList->End();
    }
  }
}
