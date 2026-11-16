/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Tools/ResourceExchangeSlot.h>

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

XII_CREATE_SIMPLE_TEST_GROUP(ExternalResourceExchange);
XII_CREATE_SIMPLE_TEST(ExternalResourceExchange, Buffer)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
  {
    return;
  }
  auto*       pProducer = environment.GetDevice();
  const auto& features  = pProducer->GetFeatures();
  if (pProducer->GetGraphicsDeviceType() != xiiGALGraphicsDeviceType::Vulkan ||
      features.m_NativeFence != xiiGALDeviceFeatureState::Enabled ||
      features.m_ExternalMemory != xiiGALDeviceFeatureState::Enabled ||
      features.m_ExternalSemaphore != xiiGALDeviceFeatureState::Enabled)
  {
    return;
  }

  xiiGALDeviceCreationDescription deviceDescription;
  deviceDescription.m_GraphicsDeviceType                 = xiiGALGraphicsDeviceType::Vulkan;
  deviceDescription.m_ValidationLevel                    = xiiGALDeviceValidationLevel::Standard;
  deviceDescription.m_DeviceFeatures.m_NativeFence       = xiiGALDeviceFeatureState::Enabled;
  deviceDescription.m_DeviceFeatures.m_ExternalMemory    = xiiGALDeviceFeatureState::Enabled;
  deviceDescription.m_DeviceFeatures.m_ExternalSemaphore = xiiGALDeviceFeatureState::Enabled;
  auto pConsumer                                         = xiiGALDeviceFactory::CreateDevice("Vulkan", xiiFoundation::GetDefaultAllocator(), deviceDescription);
  if (!XII_TEST_BOOL(pConsumer != nullptr) || !XII_TEST_RESULT(pConsumer->Initialize()))
  {
    return;
  }

  xiiGALBufferCreationDescription description;
  description.m_uiSize                 = sizeof(xiiUInt32) * 4;
  description.m_BindFlags              = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                   = xiiGALBufferMode::Structured;
  description.m_uiElementByteStride    = sizeof(xiiUInt32);
  description.m_ExternalMemory.m_Flags = xiiGALExternalMemoryFlags::BinarySemaphore;
  auto pExported                       = pProducer->CreateBuffer(description, nullptr, xiiGALExternalMemoryKind::Exportable);
  if (!XII_TEST_BOOL(pExported != nullptr))
  {
    return;
  }
  xiiGALResource* pResource    = pExported;
  description.m_ExternalMemory = pResource->GetExternalMemoryDescription();
  auto pImported               = pConsumer->CreateBuffer(description, nullptr, xiiGALExternalMemoryKind::Imported);
  if (!XII_TEST_BOOL(pImported != nullptr))
  {
    return;
  }

  description.m_ExternalMemory = {};
  description.m_BindFlags      = xiiGALBindFlags::None;
  description.m_Usage          = xiiGALResourceUsage::Staging;
  description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
  auto  pReadback              = pConsumer->CreateBuffer(description);
  auto  pProduce               = pProducer->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Compute});
  auto  pConsume               = pConsumer->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Transfer});
  auto* pProducerQueue         = pProducer->GetCommandQueue(xiiGALCommandQueueFlags::Compute);
  auto* pConsumerQueue         = pConsumer->GetCommandQueue(xiiGALCommandQueueFlags::Transfer);
  if (!XII_TEST_BOOL(pReadback && pProduce && pConsume))
  {
    return;
  }

  xiiGALResourceExchangeSlot slots[2];
  for (auto& slot : slots)
  {
    slot.BeginProduction(0);
  }
  slots[0].QueuePublication(1, 4);
  slots[1].QueuePublication(2, 5);
  XII_TEST_BOOL(!slots[0].IsReadyToPublish(3));
  slots[0].Publish(4);
  slots[1].Publish(5);
  XII_TEST_BOOL(!slots[0].MatchesPublication(2));
  XII_TEST_BOOL(slots[0].MatchesPublication(1));
  slots[0].Retire(7);
  XII_TEST_BOOL(!slots[0].IsAvailable(6));
  XII_TEST_BOOL(!slots[1].IsAvailable(7));
  XII_TEST_BOOL(slots[0].IsAvailable(7));

  for (xiiUInt32 uiCycle = 0; uiCycle < 8; ++uiCycle)
  {
    const xiiUInt32 uiPattern[] = {uiCycle, 32U + uiCycle, 64U + uiCycle, 96U + uiCycle};
    pProduce->Begin();
    if (uiCycle > 0)
    {
      XII_TEST_RESULT(pResource->AcquireExternalMemory(pProduce));
    }
    pProduce->UpdateBuffer(pExported, 0, xiiMakeArrayPtr(uiPattern).ToByteArray());
    XII_TEST_RESULT(pResource->ReleaseExternalMemory(pProduce));
    pProduce->End();
    pProducerQueue->WaitForFenceValue(pProducerQueue->Submit(pProduce));

    pConsume->Begin();
    XII_TEST_RESULT(pImported->AcquireExternalMemory(pConsume));
    pConsume->CopyBuffer(pImported, pReadback);
    XII_TEST_RESULT(pImported->ReleaseExternalMemory(pConsume));
    pConsume->End();
    pConsumerQueue->WaitForFenceValue(pConsumerQueue->Submit(pConsume));

    pConsume->Begin();
    void* pData = nullptr;
    if (XII_TEST_RESULT(pConsume->MapBuffer(pReadback, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, pData)) && XII_TEST_BOOL(pData != nullptr))
    {
      const auto* pValues = static_cast<const xiiUInt32*>(pData);
      for (xiiUInt32 uiIndex = 0; uiIndex < XII_ARRAY_SIZE(uiPattern); ++uiIndex)
      {
        XII_TEST_INT(pValues[uiIndex], uiPattern[uiIndex]);
      }
      XII_TEST_RESULT(pConsume->UnmapBuffer(pReadback, xiiGALMapType::Read));
    }
    pConsume->End();
    pConsume->Reset();
  }
}
