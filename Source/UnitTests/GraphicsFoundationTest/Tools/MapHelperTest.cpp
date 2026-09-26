/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

XII_CREATE_SIMPLE_TEST_GROUP(Tools);

XII_CREATE_SIMPLE_TEST(Tools, MapHelper)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RAII mapping and move ownership")
  {
    struct MappedValues
    {
      xiiUInt32 m_uiA;
      xiiUInt32 m_uiB;
      float     m_fValue;
    };

    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      xiiGPUTestingEnvironment environment(xiiGetGPUTestingEnvironmentName(uiImplementation));
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALBufferCreationDescription bufferDescription;
      bufferDescription.m_uiSize         = sizeof(MappedValues);
      bufferDescription.m_Usage          = xiiGALResourceUsage::Staging;
      bufferDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      xiiSharedPtr<xiiGALBuffer> pBuffer = environment.GetDevice()->CreateBuffer(bufferDescription);
      XII_TEST_BOOL(pBuffer != nullptr);
      if (pBuffer == nullptr)
        continue;

      xiiGALCommandListCreationDescription commandListDescription;
      commandListDescription.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer;
      xiiSharedPtr<xiiGALCommandList> pCommandList = environment.GetDevice()->CreateCommandList(commandListDescription);
      XII_TEST_BOOL(pCommandList != nullptr);
      if (pCommandList == nullptr)
        continue;

      pCommandList->Begin();
      {
        xiiGALMapHelper<MappedValues> mapped(pCommandList.Borrow(), pBuffer.Borrow(), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        XII_TEST_BOOL(mapped.GetMappedData() != nullptr);
        XII_TEST_INT(mapped->m_uiA, 0U);
        XII_TEST_INT(mapped->m_uiB, 0U);
        XII_TEST_FLOAT(mapped->m_fValue, 0.0f, 0.0f);

        mapped->m_uiA   = 0x10203040U;
        mapped->m_uiB   = 0xA0B0C0D0U;
        mapped->m_fValue = 42.5f;

        xiiGALMapHelper<MappedValues> moved(std::move(mapped));
        XII_TEST_BOOL(mapped.GetMappedData() == nullptr);
        XII_TEST_BOOL(moved.GetMappedData() != nullptr);
        XII_TEST_INT(moved->m_uiA, 0x10203040U);
        XII_TEST_INT(moved->m_uiB, 0xA0B0C0D0U);
        XII_TEST_FLOAT(moved->m_fValue, 42.5f, 0.0f);
        XII_TEST_BOOL(moved.Unmap().Succeeded());
        XII_TEST_BOOL(moved.GetMappedData() == nullptr);
        XII_TEST_BOOL(moved.Unmap().Succeeded());
      }
      pCommandList->End();
    }
  }
}
