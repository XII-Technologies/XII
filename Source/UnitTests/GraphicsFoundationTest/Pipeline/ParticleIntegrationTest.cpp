/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Particles/ParticleSystem.h>
#include <GraphicsCore/Pipeline/Passes/Particles/ParticleIntegrationConstantsPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

/// Test-owned initialized state, deliberately longer than both the active and dispatch ranges.
class xiiParticleIntegrationTestSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleIntegrationTestSource, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_Output{false, xiiGALBindFlags::UnorderedAccess};
  xiiSharedPtr<xiiGALBuffer>       m_Buffer;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override
  {
    if (!m_Buffer)
    {
      xiiHybridArray<xiiParticleGPUState, 131> particles;
      particles.SetCount(131);
      xiiMemoryUtils::ZeroFill(particles.GetData(), particles.GetCount());
      for (xiiUInt32 i = 0; i < particles.GetCount(); ++i)
      {
        particles[i].m_vPositionAge            = xiiVec4(static_cast<float>(i), 3, 0, i == 1 ? 9.99f : 0.1f);
        particles[i].m_vPreviousPositionRadius = xiiVec4(-1, -2, -3, 0.25f);
        particles[i].m_vVelocityLifetime       = xiiVec4(2, 0, 0, 10);
        particles[i].m_vColor                  = xiiVec4(1, 0.5f, 0.25f, 1);
        particles[i].m_uiId                    = 1000 + i;
      }
      xiiGALBufferCreationDescription description;
      description.m_uiSize              = particles.GetCount() * sizeof(xiiParticleGPUState);
      description.m_uiElementByteStride = sizeof(xiiParticleGPUState);
      description.m_Mode                = xiiGALBufferMode::Structured;
      description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
      xiiGALBufferData initial;
      initial.m_pData      = particles.GetData();
      initial.m_uiDataSize = description.m_uiSize;
      m_Buffer             = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description, &initial);
      if (!m_Buffer)
        return XII_FAILURE;
    }
    const auto emitted = context.m_Graph.AddPass<xiiRenderGraphBufferHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](auto& data, xiiRenderGraphBuilder& builder) { data = builder.ImportBuffer(context.m_sInstanceScope, m_Buffer, m_Buffer->GetResourceState()); }, [](const auto&, xiiRenderGraphPassContext&) {});
    return context.SetOutput(m_Output, *emitted.first);
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
  const auto                          source = leaf.AddPass("Initialized particle state", "xiiParticleIntegrationTestSource");
  xiiParticleIntegrationConstantsPass authored;
  authored.m_fDeltaTimeS      = 0.05f;
  authored.m_fGravityScale    = 0;
  authored.m_fDragCoefficient = 0;
  authored.m_WindVelocity     = xiiVec3(0, 2, 0);
  authored.m_uiActiveCount    = 2;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(authored.GetProperties(properties));
  const auto constants = leaf.AddPass("Parameters", "xiiParticleIntegrationConstantsPass", properties);
  properties.Clear();
  properties.Insert("ParticleCount", 130U);
  const auto integrate = leaf.AddPass("Integrate", "xiiParticleIntegratePass", properties);
  const auto readback  = leaf.AddPass("Read state", "xiiParticleIntegrationTestReadback");
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
  descriptor.AddPass("Initialized state", "xiiParticleIntegrationTestSource");
  xiiParticleIntegrationConstantsPass authored;
  authored.m_fDeltaTimeS      = 0.05f;
  authored.m_fGravityScale    = 0;
  authored.m_fDragCoefficient = 0;
  authored.m_uiActiveCount    = 2;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(authored.GetProperties(properties));
  descriptor.AddPass("Parameters", "xiiParticleIntegrationConstantsPass", properties);
  descriptor.AddPass("Simulate", "xiiParticleIntegratePass");
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
