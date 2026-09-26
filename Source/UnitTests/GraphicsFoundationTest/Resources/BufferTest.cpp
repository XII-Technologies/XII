/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Resources/Buffer.h>

XII_CREATE_SIMPLE_TEST_GROUP(Resources);

XII_CREATE_SIMPLE_TEST(Resources, Buffer)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Structured buffer and default views")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALBufferCreationDescription description;
      description.m_uiSize              = 1024U;
      description.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      description.m_Usage               = xiiGALResourceUsage::Mutable;
      description.m_Mode                = xiiGALBufferMode::Structured;
      description.m_uiElementByteStride = 16U;

      xiiSharedPtr<xiiGALBuffer> pBuffer = environment.GetDevice()->CreateBuffer(description);
      XII_TEST_BOOL(pBuffer != nullptr);
      if (pBuffer == nullptr)
        continue;

      XII_TEST_BOOL(pBuffer->GetDescription() == description);
      XII_TEST_INT(pBuffer->GetSize(), 1024U);
      XII_TEST_INT(pBuffer->GetMemoryConsumption(), 1024U);
      XII_TEST_BOOL(pBuffer->GetDevice().Borrow() == environment.GetDevice());
      XII_TEST_BOOL(pBuffer->GetExternalMemoryKind().IsNoFlagSet());
      XII_TEST_BOOL(pBuffer->IsInKnownState());
      XII_TEST_BOOL(pBuffer->CheckState(xiiGALResourceStateFlags::UnorderedAccess));

      pBuffer->SetResourceState(xiiGALResourceStateFlags::ShaderResource | xiiGALResourceStateFlags::UnorderedAccess);
      XII_TEST_BOOL(pBuffer->IsInKnownState());
      XII_TEST_BOOL(pBuffer->CheckState(xiiGALResourceStateFlags::ShaderResource));
      XII_TEST_BOOL(pBuffer->CheckAnyState(xiiGALResourceStateFlags::UnorderedAccess | xiiGALResourceStateFlags::CopySource));

      xiiSharedPtr<xiiGALBufferView> pSRV = pBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource);
      xiiSharedPtr<xiiGALBufferView> pUAV = pBuffer->GetDefaultView(xiiGALBufferViewType::UnorderedAccess);
      XII_TEST_BOOL(pSRV != nullptr);
      XII_TEST_BOOL(pUAV != nullptr);
      XII_TEST_BOOL(pSRV->GetBuffer() == pBuffer);
      XII_TEST_BOOL(pUAV->GetBuffer() == pBuffer);
      XII_TEST_INT(pSRV->GetDescription().m_uiByteWidth, 1024U);
      XII_TEST_INT(pUAV->GetDescription().m_uiByteWidth, 1024U);
    }
  }
}
