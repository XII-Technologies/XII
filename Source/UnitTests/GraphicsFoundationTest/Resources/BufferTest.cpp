/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Resources/Buffer.h>

XII_CREATE_SIMPLE_TEST_GROUP(Resources);

XII_CREATE_SIMPLE_TEST(Resources, Buffer)
{
  xiiGPUTestingEnvironment environment;
  XII_TEST_BOOL(environment.Initialize().Succeeded());
  const auto initialDeviceReferences = environment.GetDevice()->GetRefCount();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Structured buffer and default views")
  {
    xiiGALBufferCreationDescription description;
    description.m_uiSize              = 1024U;
    description.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    description.m_Usage               = xiiGALResourceUsage::Mutable;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_uiElementByteStride = 16U;

    xiiSharedPtr<xiiGALBuffer> pBuffer = environment.GetDevice()->CreateBuffer(description);
    XII_TEST_BOOL(pBuffer != nullptr);
    XII_TEST_INT(pBuffer->GetRefCount(), 1);

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
    XII_TEST_BOOL(pSRV == pBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
    XII_TEST_INT(pSRV->GetRefCount(), 1);
    pSRV.Clear();
    XII_TEST_INT(pBuffer->GetRefCount(), 2);
    pSRV = pBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource);
    XII_TEST_BOOL(pSRV != nullptr);
    pBuffer.Clear();
    XII_TEST_INT(pSRV->GetBuffer()->GetSize(), 1024);
    XII_TEST_BOOL(pSRV->GetBuffer() == pUAV->GetBuffer());
    pUAV.Clear();
    pSRV.Clear();
  }
  environment.GetDevice()->WaitIdle();
  XII_TEST_INT(environment.GetDevice()->GetRefCount(), initialDeviceReferences);
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Last view outlives the device owner")
  {
    xiiGALBufferCreationDescription description;
    description.m_uiSize              = 64;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_uiElementByteStride = 16;
    description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    auto buffer                       = environment.GetDevice()->CreateBuffer(description);
    if (!XII_TEST_BOOL(buffer != nullptr))
      return;
    auto view = buffer->GetDefaultView(xiiGALBufferViewType::ShaderResource);
    if (!XII_TEST_BOOL(view != nullptr))
      return;
    buffer.Clear();
    environment.Shutdown();
    XII_TEST_INT(view->GetBuffer()->GetSize(), 64);
    XII_TEST_BOOL(view->GetDevice() != nullptr);
    view.Clear(); // Also destroys the resource and its device; their allocator must still exist.
  }
}
