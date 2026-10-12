/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

namespace PipelineExecutionTest
{
  struct UploadData
  {
    XII_DECLARE_POD_TYPE();
    xiiRenderGraphBufferHandle m_hBuffer;
    xiiUInt32                  m_uiValue;
  };
  struct CopyData
  {
    XII_DECLARE_POD_TYPE();
    xiiRenderGraphBufferHandle m_hSource, m_hTarget;
  };
} // namespace PipelineExecutionTest

class xiiPipelineGPUUpload : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPipelineGPUUpload, xiiRenderPipelinePass);

public:
  xiiUInt32 m_uiBuildCount = 0;
  void      GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override { pins.PushBack({"Data", xiiRenderPipelinePin::Buffer, false}); }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    const xiiVariant* seed = context.m_Parameters.GetValue("Seed");
    if (!seed)
      return XII_FAILURE;
    const xiiUInt32 value   = seed->ConvertTo<xiiUInt32>() + ++m_uiBuildCount;
    auto            emitted = context.m_Graph.AddPass<PipelineExecutionTest::UploadData>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](auto& data, xiiRenderGraphBuilder& builder) {
        xiiGALBufferCreationDescription description;
        description.m_uiSize = sizeof(xiiUInt32);
        description.m_Usage = xiiGALResourceUsage::Mutable;
        description.m_BindFlags = xiiGALBindFlags::VertexBuffer;
        data.m_hBuffer = builder.WriteBuffer("PipelineUpload", description, xiiGALResourceStateFlags::CopyDestination);
        data.m_uiValue = value; }, [](const auto& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_hBuffer), 0, xiiConstByteArrayPtr(reinterpret_cast<const xiiUInt8*>(&data.m_uiValue), sizeof(data.m_uiValue))); });
    return context.SetOutput("Data", emitted.first->m_hBuffer);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPipelineGPUUpload, 1, xiiRTTIDefaultAllocator<xiiPipelineGPUUpload>)
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiPipelineGPUReadback : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPipelineGPUReadback, xiiRenderPipelinePass);

public:
  xiiSharedPtr<xiiGALBuffer> m_pReadback;
  void                       GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override { pins.PushBack({"Data", xiiRenderPipelinePin::Buffer, true}); }
  xiiResult                  Build(xiiRenderPipelinePassContext& context) override
  {
    if (!m_pReadback)
    {
      xiiGALBufferCreationDescription description;
      description.m_uiSize         = sizeof(xiiUInt32);
      description.m_Usage          = xiiGALResourceUsage::Staging;
      description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
      m_pReadback                  = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
      if (!m_pReadback)
        return XII_FAILURE;
    }
    const auto input = context.GetInputBuffer("Data");
    context.m_Graph.AddPass<PipelineExecutionTest::CopyData>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](auto& data, xiiRenderGraphBuilder& builder) {
        data.m_hSource = builder.ReadBuffer(input, xiiGALResourceStateFlags::CopySource);
        data.m_hTarget = builder.WriteBuffer(builder.ImportBuffer("PipelineReadback", m_pReadback, m_pReadback->GetResourceState()), xiiGALResourceStateFlags::CopyDestination);
        builder.ExportBuffer(data.m_hTarget, xiiGALResourceStateFlags::CopyDestination); }, [](const auto& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().CopyBuffer(execute.GetBuffer(data.m_hSource), execute.GetBuffer(data.m_hTarget)); });
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPipelineGPUReadback, 1, xiiRTTIDefaultAllocator<xiiPipelineGPUReadback>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST_GROUP(PipelineResources);
XII_CREATE_SIMPLE_TEST(PipelineResources, ResourceExecution)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(
    environment.GetDevice()->WaitIdle();
    xiiStartup::ShutdownHighLevelSystems();
    xiiGALDevice::SetDefaultDevice(nullptr);
    environment.Shutdown(););

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Two views share a resource and execute isolated state through the render graph")
  {
    xiiRenderPipelineResourceDescriptor descriptor;
    descriptor.AddPass("Readback", "xiiPipelineGPUReadback");
    descriptor.AddPass("Upload", "xiiPipelineGPUUpload");
    XII_TEST_RESULT(descriptor.Connect(1, "Data", 0, "Data"));
    descriptor.m_Parameters.Insert("Seed", 10U);
    auto                resource = xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>("Pipeline GPU Execution", std::move(descriptor));
    xiiWorldDescription worldDescription("Pipeline Execution Test");
    xiiWorld            world(worldDescription);
    XII_LOCK(world.GetWriteMarker());
    auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
    xiiCamera camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 100.0f);
    xiiView*      views[2];
    xiiViewHandle handles[2];
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      handles[i] = module->CreateView(i == 0 ? "A" : "B", views[i]);
      views[i]->SetCamera(&camera);
      views[i]->SetViewport(xiiRectFloat(0, 0, 32, 32));
      views[i]->SetRenderPipelineResource(resource);
    }
    views[1]->SetPipelineParameter("Seed", 100U);
    for (xiiUInt32 frame = 1; frame <= 3; ++frame)
    {
      world.Update();
      environment.GetDevice()->WaitIdle();
      for (xiiUInt32 i = 0; i < 2; ++i)
      {
        auto* graph    = views[i]->GetRenderGraph();
        auto  readback = graph->GetExportedBuffer(graph->FindBuffer("PipelineReadback"));
        if (!XII_TEST_BOOL(readback != nullptr))
          continue;
        xiiGALCommandListCreationDescription commands;
        commands.m_QueueFlags = xiiGALCommandQueueFlags::Graphics;
        auto mapping          = environment.GetDevice()->CreateCommandList(commands);
        mapping->Begin();
        void* data = nullptr;
        if (XII_TEST_RESULT(mapping->MapBuffer(readback.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, data)))
        {
          XII_TEST_INT(*static_cast<const xiiUInt32*>(data), (i == 0 ? 10U : 100U) + frame);
          XII_TEST_RESULT(mapping->UnmapBuffer(readback.Borrow(), xiiGALMapType::Read));
        }
        mapping->End();
      }
    }
    for (auto handle : handles)
      module->DestroyView(handle);
  }
}

XII_CREATE_SIMPLE_TEST_GROUP(PipelineDefault);
XII_CREATE_SIMPLE_TEST(PipelineDefault, OffscreenRendering)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(
    environment.GetDevice()->WaitIdle();
    xiiStartup::ShutdownHighLevelSystems();
    xiiGALDevice::SetDefaultDevice(nullptr);
    environment.Shutdown(););

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Default pipeline compiles and renders to an off-screen target")
  {
    xiiGALTextureCreationDescription description;
    description.m_Type      = xiiGALResourceDimension::Texture2D;
    description.m_Size      = xiiSizeU32(32, 32);
    description.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    description.m_BindFlags = xiiGALBindFlags::RenderTarget;
    auto target             = environment.GetDevice()->CreateTexture(description);
    if (!XII_TEST_BOOL(target != nullptr))
      return;
    xiiWorldDescription worldDescription("Default Pipeline Test");
    xiiWorld            world(worldDescription);
    XII_LOCK(world.GetWriteMarker());
    auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
    xiiCamera camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 100.0f);
    xiiView*   view   = nullptr;
    const auto handle = module->CreateView("Default", view);
    view->SetCamera(&camera);
    view->SetViewport(xiiRectFloat(0, 0, 32, 32));
    view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget).Borrow());
    const float                         viewBloomStrength = view->GetDisplayOutputSettings().m_fBloomStrength;
    xiiRenderPipelineResourceDescriptor definition;
    xiiRenderPipelineResource::GetDefaultDescriptor(definition);
    xiiVariantDictionary display;
    display.Insert("BloomStrength", 0.2f);
    definition.m_Parameters.Insert("DisplayOutput", display);
    view->SetRenderPipelineResource(xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>("Configured default pipeline", std::move(definition)));
    xiiGALTextureReadback readback(environment.GetDevice());
    for (xiiUInt32 frame = 0; frame < 2; ++frame)
    {
      if (frame == 1)
      {
        display.Insert("BloomStrength", 0.1f);
        view->SetPipelineParameter("DisplayOutput", display);
      }
      world.Update();
      XII_TEST_FLOAT(view->GetDisplayOutputSettings().m_fBloomStrength, viewBloomStrength, 0.0001f);
      environment.GetDevice()->WaitIdle();
      XII_TEST_BOOL(view->GetRenderGraph()->GetStatistics().m_uiCompiledPassCount > 10);
      XII_TEST_BOOL(view->GetRenderGraph()->GetExportedTexture(view->GetRenderGraph()->FindTexture("ViewOutput")) == target);
      xiiGALCommandListCreationDescription commands;
      commands.m_QueueFlags = xiiGALCommandQueueFlags::Graphics;
      auto commandList      = environment.GetDevice()->CreateCommandList(commands);
      commandList->Begin();
      xiiGALTextureReadback::ReadbackRequest request;
      request.m_pTexture = target.Borrow();
      XII_TEST_RESULT(readback.Enqueue(commandList.Borrow(), request));
      commandList->End();
      auto* queue = environment.GetDevice()->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
      queue->WaitForFenceValue(queue->Submit(commandList.Borrow()));
      readback.WaitForNextCompleted();
      auto capture = readback.GetCompleted();
      if (XII_TEST_BOOL(static_cast<bool>(capture)))
      {
        commandList->Begin();
        xiiGALMappedTextureSubresource mapped;
        xiiGALTextureMipLevelData      mip;
        if (XII_TEST_RESULT(commandList->MapTextureSubresource(capture.m_pStagingTexture, mip, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
        {
          for (xiiUInt32 y = 0; y < 32; ++y)
          {
            const auto* row = xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt8*>(mapped.m_pData), y * mapped.m_uiStride);
            for (xiiUInt32 x = 0; x < 32; ++x)
              XII_TEST_INT(row[x * 4 + 3], 255);
          }
          XII_TEST_RESULT(commandList->UnmapTextureSubresource(capture.m_pStagingTexture, mip));
        }
        commandList->End();
        readback.RecycleStagingTexture(capture.m_pStagingTexture);
      }
    }
    module->DestroyView(handle);
  }
}
