/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Resources/Buffer.h>

XII_CREATE_SIMPLE_TEST(Resources, BufferView)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Formatted subrange view")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALBufferCreationDescription bufferDescription;
      bufferDescription.m_uiSize              = 256U;
      bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
      bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
      bufferDescription.m_Mode                = xiiGALBufferMode::Formatted;
      bufferDescription.m_uiElementByteStride = 4U;
      xiiSharedPtr<xiiGALBuffer> pBuffer      = environment.GetDevice()->CreateBuffer(bufferDescription);
      XII_TEST_BOOL(pBuffer != nullptr);
      if (pBuffer == nullptr)
        continue;

      xiiGALBufferViewCreationDescription viewDescription;
      viewDescription.m_ViewType     = xiiGALBufferViewType::ShaderResource;
      viewDescription.m_Format       = xiiGALResourceFormat::R32Float;
      viewDescription.m_uiByteOffset = 64U;
      viewDescription.m_uiByteWidth  = 0U;

      xiiSharedPtr<xiiGALBufferView> pView = pBuffer->CreateView(viewDescription);
      XII_TEST_BOOL(pView != nullptr);
      if (pView != nullptr)
      {
        XII_TEST_INT(viewDescription.m_uiByteWidth, 192U);
        XII_TEST_BOOL(pView->GetDescription() == viewDescription);
        XII_TEST_BOOL(pView->GetBuffer() == pBuffer);
        XII_TEST_BOOL(pView->GetDevice().Borrow() == environment.GetDevice());
        pView->SetDebugName("Formatted Buffer View");
        XII_TEST_STRING(pView->GetDebugName(), "Formatted Buffer View");
      }
    }
  }
}
