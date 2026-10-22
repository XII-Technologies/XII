/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>

namespace TemporalPipelineTest
{
  struct Data
  {
    xiiRenderGraphBufferHandle m_Source, m_Target;
    xiiUInt32                  m_uiValue = 0;
  };
} // namespace TemporalPipelineTest

class xiiPipelineTemporalCounter : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPipelineTemporalCounter, xiiRenderPipelinePass);

public:
  xiiSharedPtr<xiiGALBuffer> m_Buffers[2];
  xiiSharedPtr<xiiGALBuffer> m_pReadback;
  xiiUInt32                  m_uiBuilds      = 0;
  xiiUInt32                  m_uiPrevious    = 0;
  xiiUInt32                  m_uiNext        = 0;
  bool                       m_bHistory      = false;
  bool                       m_bReadPrevious = false;
  void                       InvalidateTemporalHistory() override { m_bHistory = false; }
  void                       GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Previous", xiiRenderPipelinePin::Buffer, true, true});
    xiiRenderPipelinePin history{"History", xiiRenderPipelinePin::Buffer, false, true};
    history.m_bTemporalHistoryOutput = true;
    pins.PushBack(history);
  }
  xiiResult BuildTemporalHistory(xiiRenderPipelinePassContext& context) override
  {
    if (!m_bHistory)
      return XII_SUCCESS;
    const auto previous = m_Buffers[m_uiPrevious];
    const auto imported = context.m_Graph.AddPass<TemporalPipelineTest::Data>("Import Previous", xiiGALCommandQueueFlags::Transfer, [previous](auto& data, xiiRenderGraphBuilder& builder) { data.m_Source = builder.ImportBuffer("Previous Value", previous, previous->GetResourceState()); }, [](const auto&, xiiRenderGraphPassContext&) {});
    return context.SetOutput("History", imported.first->m_Source);
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    if (!m_Buffers[0])
    {
      xiiGALBufferCreationDescription description;
      description.m_uiSize    = sizeof(xiiUInt32);
      description.m_Usage     = xiiGALResourceUsage::Mutable;
      description.m_BindFlags = xiiGALBindFlags::VertexBuffer;
      for (auto& buffer : m_Buffers)
      {
        buffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
        if (!buffer)
          return XII_FAILURE;
      }
      description.m_Usage          = xiiGALResourceUsage::Staging;
      description.m_BindFlags      = {};
      description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
      m_pReadback                  = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
      if (!m_pReadback)
        return XII_FAILURE;
    }
    const auto previous = context.GetInputBuffer("Previous");
    m_bReadPrevious     = previous.IsValid();
    if (m_bReadPrevious)
    {
      const auto readback = m_pReadback;
      context.m_Graph.AddPass<TemporalPipelineTest::Data>("Read Previous", xiiGALCommandQueueFlags::Transfer, [previous, readback](auto& data, xiiRenderGraphBuilder& builder) {
        data.m_Source = builder.ReadBuffer(previous, xiiGALResourceStateFlags::CopySource);
        data.m_Target = builder.WriteBuffer(builder.ImportBuffer("Readback", readback, readback->GetResourceState()), xiiGALResourceStateFlags::CopyDestination);
        builder.ExportBuffer(data.m_Target, xiiGALResourceStateFlags::CopyDestination); }, [](const auto& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().CopyBuffer(execute.GetBuffer(data.m_Source), execute.GetBuffer(data.m_Target)); });
    }
    const auto* seed = context.m_Parameters.GetValue("Seed");
    if (!seed)
      return XII_FAILURE;
    const auto value      = seed->ConvertTo<xiiUInt32>() + ++m_uiBuilds;
    const auto writeIndex = m_uiNext;
    const auto target     = m_Buffers[writeIndex];
    context.m_Graph.AddPass<TemporalPipelineTest::Data>("Write Next", xiiGALCommandQueueFlags::Transfer, [target, value](auto& data, xiiRenderGraphBuilder& builder) {
      data.m_Target = builder.WriteBuffer(builder.ImportBuffer("Next Value", target, target->GetResourceState()), xiiGALResourceStateFlags::CopyDestination);
      data.m_uiValue = value; }, [this, writeIndex](const auto& data, xiiRenderGraphPassContext& execute) {
      execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Target), 0, xiiConstByteArrayPtr(reinterpret_cast<const xiiUInt8*>(&data.m_uiValue), sizeof(data.m_uiValue)));
      m_uiPrevious = writeIndex;
      m_uiNext = 1 - writeIndex;
      m_bHistory = true; }, true);
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPipelineTemporalCounter, 1, xiiRTTIDefaultAllocator<xiiPipelineTemporalCounter>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST_GROUP(PipelineTemporal);
XII_CREATE_SIMPLE_TEST(PipelineTemporal, InstanceHistory)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Self feedback reads previous-frame GPU resources per occurrence and view")
  {
    xiiRenderPipelineResourceDescriptor leaf;
    leaf.AddPass("Counter", xiiGetStaticRTTI<xiiPipelineTemporalCounter>());
    XII_TEST_RESULT(leaf.Connect(0, "History", 0, "Previous"));
    xiiGraphDefinition definition;
    XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
    xiiRenderPipelineResourceDescriptor root;
    root.m_Definitions.PushBack(definition);
    root.AddSubgraph("First", leaf.m_Id);
    root.AddSubgraph("Second", leaf.m_Id);
    root.m_Parameters.Insert("Seed", 10U);
    root.m_Passes[1].m_Parameters.Insert("Seed", 100U);
    const auto          resource = xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>("Temporal Pipeline", std::move(root));
    xiiWorldDescription description("Temporal Pipeline Test");
    xiiWorld            world(description);
    XII_LOCK(world.GetWriteMarker());
    auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
    xiiCamera camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    xiiView* views[2];
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      module->CreateView(i == 0 ? "A" : "B", views[i]);
      views[i]->SetCamera(&camera);
      views[i]->SetViewport(xiiRectFloat(0, 0, 16, 16));
      views[i]->SetRenderPipelineResource(resource);
    }
    for (xiiUInt32 frame = 1; frame <= 4; ++frame)
    {
      views[1]->SetRenderEnabled(frame != 2);
      if (frame == 3)
        views[0]->InvalidateTemporalHistory();
      world.Update();
      environment.GetDevice()->WaitIdle();
      for (xiiUInt32 view = 0; view < 2; ++view)
      {
        if (view == 1 && frame == 2)
          continue;
        const auto& instance = views[view]->GetRenderPipelineInstance();
        XII_TEST_INT(instance.GetPipeline()->GetTopology().GetEdgeCount(), 0);
        const auto* first  = static_cast<const xiiPipelineTemporalCounter*>(instance.GetPass(0));
        const auto* second = static_cast<const xiiPipelineTemporalCounter*>(instance.GetPass(1));
        XII_TEST_BOOL(first->m_Buffers[0] != second->m_Buffers[0]);
        for (xiiUInt32 occurrence = 0; occurrence < 2; ++occurrence)
        {
          const auto* counter         = occurrence == 0 ? first : second;
          const bool  expectedHistory = frame > 1 && !(view == 0 && frame == 3);
          XII_TEST_BOOL(counter->m_bReadPrevious == expectedHistory);
          if (!counter->m_bReadPrevious)
            continue;
          xiiGALCommandListCreationDescription commands;
          commands.m_QueueFlags = xiiGALCommandQueueFlags::Graphics;
          auto mapping          = environment.GetDevice()->CreateCommandList(commands);
          mapping->Begin();
          void* data = nullptr;
          if (XII_TEST_RESULT(mapping->MapBuffer(counter->m_pReadback.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, data)))
          {
            XII_TEST_INT(*static_cast<const xiiUInt32*>(data), (occurrence == 0 ? 10U : 100U) + counter->m_uiBuilds - 1);
            XII_TEST_RESULT(mapping->UnmapBuffer(counter->m_pReadback.Borrow(), xiiGALMapType::Read));
          }
          mapping->End();
        }
      }
    }
    const auto* a = static_cast<const xiiPipelineTemporalCounter*>(views[0]->GetRenderPipelineInstance().GetPass(0));
    const auto* b = static_cast<const xiiPipelineTemporalCounter*>(views[1]->GetRenderPipelineInstance().GetPass(0));
    XII_TEST_BOOL(a->m_Buffers[0] != b->m_Buffers[0]);
    for (auto* view : views)
      module->DestroyView(view->GetHandle());
  }
}
