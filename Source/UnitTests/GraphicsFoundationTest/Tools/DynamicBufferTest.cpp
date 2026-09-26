/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/Tools/DynamicBuffer.h>

XII_CREATE_SIMPLE_TEST(Tools, DynamicBuffer)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Initialization and discard resize")
  {
    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALBufferCreationDescription description;
      description.m_uiSize    = 16U;
      description.m_Usage     = xiiGALResourceUsage::Mutable;
      description.m_BindFlags = xiiGALBindFlags::VertexBuffer;

      xiiGALDynamicBuffer buffer(environment.GetDeviceShared(), description);
      XII_TEST_BOOL(buffer.GetBuffer() != nullptr);
      XII_TEST_BOOL(!buffer.PendingUpdate());
      XII_TEST_INT(buffer.GetDescription().m_uiSize, 16U);
      XII_TEST_INT(buffer.GetBuffer()->GetDescription().m_uiSize, 16U);
      const xiiUInt32 uiInitialVersion = buffer.GetVersion();
      XII_TEST_BOOL(uiInitialVersion > 0U);

      xiiSharedPtr<xiiGALBuffer> pOriginal = buffer.GetBuffer();
      xiiSharedPtr<xiiGALBuffer> pResized = buffer.Resize(nullptr, 64U, true);
      XII_TEST_BOOL(pResized != nullptr);
      XII_TEST_BOOL(pResized != pOriginal);
      XII_TEST_BOOL(!buffer.PendingUpdate());
      XII_TEST_INT(buffer.GetDescription().m_uiSize, 64U);
      XII_TEST_INT(pResized->GetDescription().m_uiSize, 64U);
      XII_TEST_INT(buffer.GetVersion(), uiInitialVersion + 1U);

      XII_TEST_BOOL(buffer.Update(nullptr) == pResized);
      XII_TEST_BOOL(buffer.Resize(nullptr, 0U, true) == nullptr);
      XII_TEST_BOOL(buffer.GetBuffer() == nullptr);
      XII_TEST_BOOL(!buffer.PendingUpdate());
      XII_TEST_INT(buffer.GetDescription().m_uiSize, 0U);
    }
  }
}
