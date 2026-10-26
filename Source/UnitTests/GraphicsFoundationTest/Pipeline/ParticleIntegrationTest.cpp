/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>

#include <Core/World/World.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Particles/ParticleSystem.h>
#include <GraphicsCore/Pipeline/Passes/Particles/ParticleIntegratePass.h>
#include <GraphicsCore/Pipeline/Passes/Particles/ParticleIntegrationConstantsPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>

/// Test-owned initialized state, deliberately longer than both the active and dispatch ranges.
class xiiParticleIntegrationTestSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleIntegrationTestSource, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_Output{false, xiiGALBindFlags::UnorderedAccess};
  xiiSharedPtr<xiiGALBuffer>       m_Buffer;
  bool                             m_bInitialized = false;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override
  {
    if (!m_Buffer)
    {
      xiiGALBufferCreationDescription description;
      description.m_uiSize              = 131 * sizeof(xiiParticleGPUState);
      description.m_uiElementByteStride = sizeof(xiiParticleGPUState);
      description.m_Mode                = xiiGALBufferMode::Structured;
      description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
      m_Buffer                          = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
      if (!m_Buffer)
        return XII_FAILURE;
    }
    struct Data
    {
      xiiRenderGraphBufferHandle                                       m_Buffer;
      xiiDynamicArray<xiiParticleGPUState, xiiAlignedAllocatorWrapper> m_Initial;
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Buffer = builder.ImportBuffer(context.m_sInstanceScope, m_Buffer, m_Buffer->GetResourceState());
      if (!m_bInitialized)
      {
        // The graph owns this upload and its cross-queue dependency. Implicit
        // CreateBuffer initial data on another queue would require an explicit fence.
        data.m_Buffer = builder.WriteBuffer(data.m_Buffer, xiiGALResourceStateFlags::CopyDestination);
        data.m_Initial.SetCount(131);
        xiiMemoryUtils::ZeroFill(data.m_Initial.GetData(), data.m_Initial.GetCount());
        for (xiiUInt32 i = 0; i < data.m_Initial.GetCount(); ++i)
        {
          auto& particle = data.m_Initial[i];
          particle.m_vPositionAge = xiiVec4(static_cast<float>(i), 3, 0, i == 1 ? 9.99f : 0.1f);
          particle.m_vPreviousPositionRadius = xiiVec4(-1, -2, -3, 0.25f);
          particle.m_vVelocityLifetime = xiiVec4(2, 0, 0, 10);
          particle.m_vColor = xiiVec4(1, 0.5f, 0.25f, 1);
          particle.m_uiId = 1000 + i;
        }
      } }, [this](const Data& data, xiiRenderGraphPassContext& execute) {
      if (!data.m_Initial.IsEmpty())
      {
        execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Buffer), 0, xiiConstByteArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Initial.GetData()), data.m_Initial.GetCount() * sizeof(xiiParticleGPUState)));
        m_bInitialized = true;
      } });
    return context.SetOutput(m_Output, emitted.first->m_Buffer);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleIntegrationTestSource, 1, xiiRTTIDefaultAllocator<xiiParticleIntegrationTestSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Particles", m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiParticleIntegrationTestReadback : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleIntegrationTestReadback, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin m_Input;
  xiiRenderGraphBufferHandle      m_Readback;
  xiiResult                       Build(xiiRenderPipelinePassContext& context) override
  {
    const auto  input       = context.GetInputBuffer(m_Input);
    const auto* description = context.m_Graph.GetBufferDescription(input);
    if (!description)
      return XII_FAILURE;
    const xiiUInt64 size = description->m_uiSize;
    struct Data
    {
      xiiRenderGraphBufferHandle m_Source, m_Target;
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Source = builder.ReadBuffer(input, xiiGALResourceStateFlags::CopySource);
      xiiGALBufferCreationDescription staging;
      staging.m_uiSize = size;
      staging.m_Usage = xiiGALResourceUsage::Staging;
      staging.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
      data.m_Target = builder.WriteBuffer(context.m_sPassName, staging, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_Target, xiiGALResourceStateFlags::CopyDestination); }, [](const Data& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().CopyBuffer(execute.GetBuffer(data.m_Source), execute.GetBuffer(data.m_Target)); }, true);
    m_Readback         = emitted.first->m_Target;
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleIntegrationTestReadback, 1, xiiRTTIDefaultAllocator<xiiParticleIntegrationTestReadback>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Particles", m_Input)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(PipelineResources, SuppliedParticleStateIntegration)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor leaf;
  const auto                          source = leaf.AddPass("Initialized particle state", xiiGetStaticRTTI<xiiParticleIntegrationTestSource>());
  xiiParticleIntegrationConstantsPass authored;
  authored.m_fDeltaTimeS      = 0.05f;
  authored.m_fGravityScale    = 0;
  authored.m_fDragCoefficient = 0;
  authored.m_WindVelocity     = xiiVec3(0, 2, 0);
  authored.m_uiActiveCount    = 2;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(authored.GetProperties(properties));
  const auto constants = leaf.AddPass("Parameters", xiiGetStaticRTTI<xiiParticleIntegrationConstantsPass>(), properties);
  properties.Clear();
  properties.Insert("ParticleCount", 130U);
  const auto integrate = leaf.AddPass("Integrate", xiiGetStaticRTTI<xiiParticleIntegratePass>(), properties);
  const auto readback  = leaf.AddPass("Read state", xiiGetStaticRTTI<xiiParticleIntegrationTestReadback>());
  XII_TEST_RESULT(leaf.Connect(source, "Particles", integrate, "Particles"));
  XII_TEST_RESULT(leaf.Connect(constants, "ParticleSimConstants", integrate, "ParticleSimConstants"));
  XII_TEST_RESULT(leaf.Connect(integrate, "Particles", readback, "Particles"));
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("Active range", definition.m_Id);
  root.AddSubgraph("Dispatch range", definition.m_Id);
  const auto fullRangeId = root.m_Passes[1].m_Id;
  auto&      override    = root.m_Passes[1].m_Overrides.ExpandAndGetRef();
  override.m_Path.PushBack(leaf.m_Passes[constants].m_Id);
  authored.m_uiActiveCount = 0;
  XII_TEST_RESULT(authored.GetProperties(override.m_Properties));
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Explicit particle integration", std::move(root));
  xiiWorldDescription description("Particle integration without scene ownership");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView*      views[2];
  xiiViewHandle handles[2];
  for (xiiUInt32 v = 0; v < 2; ++v)
  {
    handles[v] = module->CreateView(v ? "Second view" : "First view", views[v]);
    views[v]->SetCamera(&camera);
    views[v]->SetViewport({0, 0, 8, 8});
    views[v]->SetRenderPipelineResource(resource);
  }
  xiiSet<const xiiGALBuffer*> distinctState;
  for (xiiUInt32 frame = 1; frame <= 2; ++frame)
  {
    world.Update();
    environment.GetDevice()->WaitIdle();
    for (auto* view : views)
    {
      const auto& instance = view->GetRenderPipelineInstance();
      const auto* pipeline = instance.GetPipeline();
      if (!XII_TEST_BOOL(pipeline != nullptr))
        return;
      XII_TEST_BOOL(pipeline->GetRuntimeFeatures().IsNoFlagSet() && pipeline->GetExtractionRequirements().IsNoFlagSet());
      for (xiiUInt32 i = 0; i < pipeline->GetDescriptor().m_Passes.GetCount(); ++i)
      {
        if (const auto* owner = xiiDynamicCast<const xiiParticleIntegrationTestSource*>(instance.GetPass(i)); owner && frame == 1)
          distinctState.Insert(owner->m_Buffer.Borrow());
        const auto* output = xiiDynamicCast<const xiiParticleIntegrationTestReadback*>(instance.GetPass(i));
        if (!output)
          continue;
        const bool full   = pipeline->GetProvenance()[i].m_InstancePath[0] == fullRangeId;
        const auto buffer = view->GetRenderGraph()->GetExportedBuffer(output->m_Readback);
        if (!XII_TEST_BOOL(buffer != nullptr))
          return;
        auto commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
        commands->Begin();
        void* mapped = nullptr;
        if (XII_TEST_RESULT(commands->MapBuffer(buffer.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, mapped)))
        {
          const auto* particles = static_cast<const xiiParticleGPUState*>(mapped);
          for (xiiUInt32 index = 0; index < 131; ++index)
          {
            const auto& p = particles[index];
            XII_TEST_INT(p.m_uiId, 1000 + index);
            if (index == 1)
            {
              XII_TEST_FLOAT(p.m_vPositionAge.y, -1e6f, 0.01f);
              XII_TEST_FLOAT(p.m_vVelocityLifetime.w, 0, 0);
            }
            else
            {
              const bool updated = index < (full ? 130U : 2U);
              XII_TEST_FLOAT(p.m_vPositionAge.x, static_cast<float>(index) + (updated ? 0.1f * frame : 0), 0.0001f);
              XII_TEST_FLOAT(p.m_vPositionAge.y, 3 + (updated ? 0.005f * frame * (frame + 1) / 2 : 0), 0.0001f);
              XII_TEST_FLOAT(p.m_vPositionAge.w, 0.1f + (updated ? 0.05f * frame : 0), 0.0001f);
              XII_TEST_FLOAT(p.m_vPreviousPositionRadius.x, updated ? static_cast<float>(index) + 0.1f * (frame - 1) : -1, 0.0001f);
              XII_TEST_FLOAT(p.m_vPreviousPositionRadius.w, 0.25f, 0);
            }
          }
          XII_TEST_RESULT(commands->UnmapBuffer(buffer.Borrow(), xiiGALMapType::Read));
        }
        commands->End();
      }
    }
  }
  XII_TEST_INT(distinctState.GetCount(), 4);
  for (const auto handle : handles)
    module->DestroyView(handle);
}

XII_CREATE_SIMPLE_TEST(PipelineResources, ParticleIntegrationWithoutFrameConsumer)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor descriptor;
  descriptor.AddPass("Initialized state", xiiGetStaticRTTI<xiiParticleIntegrationTestSource>());
  xiiParticleIntegrationConstantsPass authored;
  authored.m_fDeltaTimeS      = 0.05f;
  authored.m_fGravityScale    = 0;
  authored.m_fDragCoefficient = 0;
  authored.m_uiActiveCount    = 2;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(authored.GetProperties(properties));
  descriptor.AddPass("Parameters", xiiGetStaticRTTI<xiiParticleIntegrationConstantsPass>(), properties);
  descriptor.AddPass("Simulate", xiiGetStaticRTTI<xiiParticleIntegratePass>());
  XII_TEST_RESULT(descriptor.Connect(0, "Particles", 2, "Particles"));
  XII_TEST_RESULT(descriptor.Connect(1, "ParticleSimConstants", 2, "ParticleSimConstants"));
  // No exports or rendering consumers: the update of externally owned state is observable later.
  const auto          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Particle simulation without output consumers", std::move(descriptor));
  xiiWorldDescription description("Standalone simulation operation");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiView*   view   = nullptr;
  const auto handle = module->CreateView("Simulation only", view);
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 8, 8});
  view->SetRenderPipelineResource(resource);
  world.Update();
  environment.GetDevice()->WaitIdle();
  const auto* source = xiiDynamicCast<const xiiParticleIntegrationTestSource*>(view->GetRenderPipelineInstance().GetPass(0));
  if (!XII_TEST_BOOL(source && source->m_Buffer))
    return;
  xiiGALBufferCreationDescription stagingDescription;
  stagingDescription.m_uiSize         = source->m_Buffer->GetDescription().m_uiSize;
  stagingDescription.m_Usage          = xiiGALResourceUsage::Staging;
  stagingDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
  const auto staging                  = environment.GetDevice()->CreateBuffer(stagingDescription);
  if (!XII_TEST_BOOL(staging != nullptr))
    return;
  auto commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
  commands->Begin();
  commands->CopyBuffer(source->m_Buffer.Borrow(), staging.Borrow());
  commands->End();
  auto* queue = environment.GetDevice()->GetCommandQueue();
  queue->WaitForFenceValue(queue->Submit(commands));
  commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
  commands->Begin();
  void* mapped = nullptr;
  if (XII_TEST_RESULT(commands->MapBuffer(staging.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, mapped)))
  {
    const auto* particles = static_cast<const xiiParticleGPUState*>(mapped);
    XII_TEST_FLOAT(particles[0].m_vPositionAge.x, 0.1f, 0.0001f);
    XII_TEST_FLOAT(particles[0].m_vPositionAge.w, 0.15f, 0.0001f);
    XII_TEST_FLOAT(particles[1].m_vVelocityLifetime.w, 0, 0);
    XII_TEST_FLOAT(particles[130].m_vPositionAge.w, 0.1f, 0.0001f);
    XII_TEST_RESULT(commands->UnmapBuffer(staging.Borrow(), xiiGALMapType::Read));
  }
  commands->End();
  module->DestroyView(handle);
}

/// An immutable graph-local upload exercises retirement without a fixture-owned callback.
class xiiParticleRecordingTestSource final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleRecordingTestSource, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_Particles{false, xiiGALBindFlags::UnorderedAccess};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override
  {
    struct Data
    {
      xiiRenderGraphBufferHandle                                       buffer;
      xiiDynamicArray<xiiParticleGPUState, xiiAlignedAllocatorWrapper> particles;
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [](Data& data, xiiRenderGraphBuilder& builder) {
      data.particles.SetCount(3);xiiMemoryUtils::ZeroFill(data.particles.GetData(),data.particles.GetCount());
      for(xiiUInt32 i=0;i<3;++i){auto& p=data.particles[i];p.m_vPositionAge={float(i),3,0,0.1f};p.m_vPreviousPositionRadius={0,0,0,0.25f};p.m_vVelocityLifetime={2,0,0,i==1?0.11f:10.0f};p.m_vColor={1,0.5f,0.25f,1};p.m_uiId=100+i;}
      xiiGALBufferCreationDescription description;description.m_Mode=xiiGALBufferMode::Structured;description.m_BindFlags=xiiGALBindFlags::UnorderedAccess;description.m_uiSize=data.particles.GetCount()*sizeof(xiiParticleGPUState);description.m_uiElementByteStride=sizeof(xiiParticleGPUState);data.buffer=builder.WriteBuffer("Recording particle state",description,xiiGALResourceStateFlags::CopyDestination); }, [](const Data& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.buffer), 0, xiiConstByteArrayPtr(reinterpret_cast<const xiiUInt8*>(data.particles.GetData()), data.particles.GetCount() * sizeof(xiiParticleGPUState))); });
    return context.SetOutput(m_Particles, emitted.first->buffer);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleRecordingTestSource, 1, xiiRTTIDefaultAllocator<xiiParticleRecordingTestSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Particles", m_Particles)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_CREATE_SIMPLE_TEST(PipelineResources, ParticleIntegrationRetainsPreparedRecording)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor definition;
  const auto                          source = definition.AddPass("Captured seed particles", xiiGetStaticRTTI<xiiParticleRecordingTestSource>());
  xiiParticleIntegrationConstantsPass settings;
  settings.m_fDeltaTimeS      = 0.05f;
  settings.m_fGravityScale    = 0;
  settings.m_fDragCoefficient = 0;
  settings.m_WindVelocity     = {0, 2, 0};
  xiiVariantDictionary parameters;
  XII_TEST_RESULT(settings.GetProperties(parameters));
  const auto constants   = definition.AddPass("Captured integration constants", xiiGetStaticRTTI<xiiParticleIntegrationConstantsPass>(), parameters);
  const auto integration = definition.AddPass("Retired integration owner", xiiGetStaticRTTI<xiiParticleIntegratePass>());
  const auto readback    = definition.AddPass("Retired readback owner", xiiGetStaticRTTI<xiiParticleIntegrationTestReadback>());
  XII_TEST_RESULT(definition.Connect(source, "Particles", integration, "Particles"));
  XII_TEST_RESULT(definition.Connect(constants, "ParticleSimConstants", integration, "ParticleSimConstants"));
  XII_TEST_RESULT(definition.Connect(integration, "Particles", readback, "Particles"));
  const auto                resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Particles.RetiredRecording", std::move(definition));
  xiiRenderPipelineInstance instance;
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(instance.Synchronize(resource)) || !XII_TEST_RESULT(context.Initialize("Particle recording lifetime")))
    return;
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  auto& graph = context.GetGraph();
  auto& cache = context.GetResourceCache();
  cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
  graph.BeginSetup(frame.GetFrameIndex());
  if (!XII_TEST_RESULT(instance.Build(nullptr, graph, context.GetBlackboard())))
    return;
  xiiRenderGraphBufferHandle output;
  for (xiiUInt32 i = 0; i < instance.GetPipeline()->GetDescriptor().m_Passes.GetCount(); ++i)
    if (const auto* pass = xiiDynamicCast<const xiiParticleIntegrationTestReadback*>(instance.GetPass(i)))
      output = pass->m_Readback;
  graph.EndSetup();
  instance.Clear();
  xiiStringBuilder error;
  if (!XII_TEST_RESULT_MSG(graph.Compile({}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
    return;
  environment.GetDevice()->WaitIdle();
  const auto staging = graph.GetExportedBuffer(output);
  if (!XII_TEST_BOOL(staging != nullptr))
    return;
  auto commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
  commands->Begin();
  void* mapped = nullptr;
  if (XII_TEST_RESULT(commands->MapBuffer(staging.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, mapped)))
  {
    const auto* particles = static_cast<const xiiParticleGPUState*>(mapped);
    XII_TEST_FLOAT(particles[0].m_vPositionAge.x, 0.1f, 0.00001f);
    XII_TEST_FLOAT(particles[0].m_vPositionAge.y, 3.005f, 0.00001f);
    XII_TEST_FLOAT(particles[0].m_vPositionAge.w, 0.15f, 0.00001f);
    XII_TEST_FLOAT(particles[1].m_vVelocityLifetime.w, 0, 0);
    XII_TEST_FLOAT(particles[1].m_vPositionAge.y, -1e6f, 0);
    XII_TEST_FLOAT(particles[2].m_vPositionAge.x, 2.1f, 0.00001f);
    for (xiiUInt32 i = 0; i < 3; ++i)
      XII_TEST_INT(particles[i].m_uiId, 100 + i);
    XII_TEST_RESULT(commands->UnmapBuffer(staging.Borrow(), xiiGALMapType::Read));
  }
  commands->End();
  environment.GetDevice()->GetCommandQueue(xiiGALCommandQueueFlags::Graphics)->Submit(commands);
}

class xiiParticleExtractionTestPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleExtractionTestPass, xiiRenderPipelinePass);

public:
  bool                                     m_bDemand = true;
  xiiDynamicArray<xiiParticleRenderData>   m_Snapshots;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return m_bDemand ? xiiRenderExtractionCategory::Particles : xiiRenderExtractionCategory::Geometry; }
  xiiResult                                Build(xiiRenderPipelinePassContext& context) override
  {
    m_Snapshots.Clear();
    if (!context.m_pView || !context.m_pView->GetExtractedRenderData())
      return XII_FAILURE;
    for (const auto* packet : context.m_pView->GetExtractedRenderData()->GetAllRenderData())
      if (const auto* particle = xiiDynamicCast<const xiiParticleRenderData*>(packet))
        m_Snapshots.PushBack(*particle);
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleExtractionTestPass, 1, xiiRTTIDefaultAllocator<xiiParticleExtractionTestPass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Demand", m_bDemand),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_CREATE_SIMPLE_TEST(PipelineExtraction, ImmutableParticleSettingsAndCategoryDemand)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceHandle resources[2];
  for (xiiUInt32 variant = 0; variant < 2; ++variant)
  {
    xiiRenderPipelineResourceDescriptor definition;
    xiiVariantDictionary                settings;
    settings.Insert("Demand", variant == 0);
    definition.AddPass("Capture authoritative particle packets", xiiGetStaticRTTI<xiiParticleExtractionTestPass>(), settings);
    resources[variant] = xiiResourceManager::CreateResource<xiiRenderPipelineResource>(variant ? "ParticleExtraction.OpaqueOnly" : "ParticleExtraction.Particles", std::move(definition));
  }
  xiiWorldDescription worldDescription("Particle extraction ownership");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiGameObjectDescription objectDescription;
  objectDescription.m_bDynamic       = true;
  xiiGameObject*              object = nullptr;
  const auto                  owner  = world.CreateObject(objectDescription, object);
  xiiParticleSystemComponent* system = nullptr;
  xiiParticleSystemComponent::CreateComponent(object, system);
  xiiParticleSystemDescriptor settings;
  settings.m_bAlwaysVisible = true;
  settings.m_uiMaxParticles = 32;
  system->SetDescriptor(settings);
  xiiParticleEmitterComponent* emitter = nullptr;
  xiiParticleEmitterComponent::CreateComponent(object, emitter);
  emitter->SetRadius(0.4f);
  xiiParticleRendererComponent* renderer = nullptr;
  xiiParticleRendererComponent::CreateComponent(object, renderer);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt({0, -4, 0}, {0, 0, 0}, {0, 0, 1});
  xiiView* views[3];
  for (xiiUInt32 v = 0; v < 3; ++v)
  {
    world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Particle snapshot view", views[v]);
    views[v]->SetCamera(&camera);
    views[v]->SetViewport({0, 0, 8, 8});
    views[v]->SetRenderPipelineResource(resources[v == 2 ? 1 : 0]);
  }
  xiiParticleRenderData retained;
  xiiUInt64             firstRevision = 0;
  for (xiiUInt32 phase = 0; phase < 4; ++phase)
  {
    if (phase == 1)
    {
      emitter->SetRadius(0.8f);
      const auto* property = static_cast<const xiiAbstractMemberProperty*>(renderer->GetDynamicRTTI()->FindPropertyByName("Tint"));
      xiiReflectionUtils::SetMemberPropertyValue(property, renderer, xiiColor(0.1f, 0.2f, 0.8f, 0.5f));
    }
    if (phase == 2)
    {
      system->SetActiveFlag(false);
      system->SetActiveFlag(true);
      emitter->SetActiveFlag(false);
    }
    if (phase == 3)
      system->SetActiveFlag(false);
    world.Update();
    environment.GetDevice()->WaitIdle();
    for (xiiUInt32 v = 0; v < 3; ++v)
    {
      XII_TEST_RESULT(views[v]->GetLastRenderResult());
      const auto& instance = views[v]->GetRenderPipelineInstance();
      const auto* capture  = xiiDynamicCast<const xiiParticleExtractionTestPass*>(instance.GetPass(0));
      if (!XII_TEST_BOOL(capture != nullptr))
        return;
      XII_TEST_INT(capture->m_Snapshots.GetCount(), v == 2 || phase == 3 ? 0 : 1);
      if (capture->m_Snapshots.IsEmpty())
        continue;
      const auto& packet = capture->m_Snapshots[0];
      XII_TEST_BOOL(packet.m_hOwnerObject == owner && packet.m_hOwnerComponent == system->GetHandle());
      XII_TEST_BOOL(packet.m_ElapsedTime.GetSeconds() >= 0);
      XII_TEST_INT(packet.m_uiWorldUpdate, world.GetUpdateCounter());
      XII_TEST_INT(packet.m_Emitters.GetCount(), phase == 2 ? 0 : 1);
      XII_TEST_INT(packet.m_Renderers.GetCount(), 1);
      if (phase == 0)
      {
        firstRevision = packet.m_uiSimulationRevision;
        if (v == 0)
          retained = packet;
        XII_TEST_FLOAT(packet.m_Emitters[0].m_fRadius, 0.4f, 0);
      }
      if (phase == 1)
      {
        XII_TEST_FLOAT(packet.m_Emitters[0].m_fRadius, 0.8f, 0);
        XII_TEST_FLOAT(packet.m_Renderers[0].m_Tint.b, 0.8f, 0);
      }
      if (phase == 2)
        XII_TEST_BOOL(packet.m_uiSimulationRevision > firstRevision);
    }
  }
  world.DeleteObjectNow(owner);
  XII_TEST_FLOAT(retained.m_Emitters[0].m_fRadius, 0.4f, 0);
  XII_TEST_FLOAT(retained.m_Renderers[0].m_Tint.b, 1, 0);
  XII_TEST_BOOL(retained.m_hOwnerObject == owner);
}
