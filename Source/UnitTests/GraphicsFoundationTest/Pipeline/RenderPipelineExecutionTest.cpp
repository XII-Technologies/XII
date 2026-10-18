/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
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
  xiiUInt32                        m_uiBuildCount = 0;
  xiiRenderPipelineBufferOutputPin m_OutputData;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override
  {
    const xiiVariant* seed = context.m_Parameters.GetValue("Seed");
    if (!seed)
      return XII_FAILURE;
    context.m_Blackboard.Set("ScopedSeed", seed->ConvertTo<xiiUInt32>());
    const xiiUInt32 value   = seed->ConvertTo<xiiUInt32>() + ++m_uiBuildCount;
    auto            emitted = context.m_Graph.AddPass<PipelineExecutionTest::UploadData>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](auto& data, xiiRenderGraphBuilder& builder) {
        xiiGALBufferCreationDescription description;
        description.m_uiSize = sizeof(xiiUInt32);
        description.m_Usage = xiiGALResourceUsage::Mutable;
        description.m_BindFlags = xiiGALBindFlags::VertexBuffer;
        data.m_hBuffer = builder.WriteBuffer("PipelineUpload", description, xiiGALResourceStateFlags::CopyDestination);
        data.m_uiValue = value; }, [](const auto& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_hBuffer), 0, xiiConstByteArrayPtr(reinterpret_cast<const xiiUInt8*>(&data.m_uiValue), sizeof(data.m_uiValue))); });
    return context.SetOutput(m_OutputData, emitted.first->m_hBuffer);
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPipelineGPUUpload, 1, xiiRTTIDefaultAllocator<xiiPipelineGPUUpload>)
  {
    XII_BEGIN_PROPERTIES
    {XII_MEMBER_PROPERTY("Data", m_OutputData)->AddFlags(xiiPropertyFlags::ReadOnly)} XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiPipelineGPUReadback : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPipelineGPUReadback, xiiRenderPipelinePass);

public:
  xiiSharedPtr<xiiGALBuffer>      m_pReadback;
  xiiUInt32                       m_uiObservedSeed = xiiInvalidIndex;
  xiiUInt32                       m_uiExecutedSeed = xiiInvalidIndex;
  xiiRenderPipelineBufferInputPin m_InputData;
  xiiResult                       Build(xiiRenderPipelinePassContext& context) override
  {
    XII_IGNORE_UNUSED(context.m_Blackboard.TryGet("ScopedSeed", m_uiObservedSeed));
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
    const auto input = context.GetInputBuffer(m_InputData);
    context.m_Graph.AddPass<PipelineExecutionTest::CopyData>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](auto& data, xiiRenderGraphBuilder& builder) {
        data.m_hSource = builder.ReadBuffer(input, xiiGALResourceStateFlags::CopySource);
        data.m_hTarget = builder.WriteBuffer(builder.ImportBuffer("PipelineReadback", m_pReadback, m_pReadback->GetResourceState()), xiiGALResourceStateFlags::CopyDestination);
        builder.ExportBuffer(data.m_hTarget, xiiGALResourceStateFlags::CopyDestination); }, [this](const auto& data, xiiRenderGraphPassContext& execute) {
          XII_IGNORE_UNUSED(execute.GetBlackboard().TryGet("ScopedSeed", m_uiExecutedSeed));
          execute.GetCommandList().CopyBuffer(execute.GetBuffer(data.m_hSource), execute.GetBuffer(data.m_hTarget)); });
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPipelineGPUReadback, 1, xiiRTTIDefaultAllocator<xiiPipelineGPUReadback>)
  {
    XII_BEGIN_PROPERTIES
    {XII_MEMBER_PROPERTY("Data", m_InputData)->AddFlags(xiiPropertyFlags::ReadOnly)} XII_END_PROPERTIES;
  }
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
        const auto* owner    = static_cast<const xiiPipelineGPUReadback*>(views[i]->GetRenderPipelineInstance().GetPass(0));
        auto        readback = owner ? owner->m_pReadback : xiiSharedPtr<xiiGALBuffer>();
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

XII_CREATE_SIMPLE_TEST(PipelineResources, HierarchicalExecution)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Repeated subgraphs own GPU history and blackboards in two views")
  {
    xiiRenderPipelineResourceDescriptor leaf;
    leaf.AddPass("Readback", "xiiPipelineGPUReadback");
    leaf.AddPass("Upload", "xiiPipelineGPUUpload");
    XII_TEST_RESULT(leaf.Connect(1, "Data", 0, "Data"));
    xiiGraphDefinition definition;
    XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
    xiiRenderPipelineResourceDescriptor root;
    root.m_Definitions.PushBack(definition);
    root.AddSubgraph("First", leaf.m_Id);
    root.AddSubgraph("Second", leaf.m_Id);
    root.m_Parameters.Insert("Seed", 10U);
    root.m_Passes[1].m_Parameters.Insert("Seed", 100U);
    const auto          resource = xiiResourceManager::GetOrCreateResource<xiiRenderPipelineResource>("Hierarchical GPU Execution", std::move(root));
    xiiWorldDescription description("Hierarchical Pipeline Execution Test");
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
    for (xiiUInt32 frame = 1; frame <= 3; ++frame)
    {
      views[1]->SetRenderEnabled(frame != 2);
      world.Update();
      environment.GetDevice()->WaitIdle();
      for (xiiUInt32 view = 0; view < 2; ++view)
      {
        if (view == 1 && frame == 2)
          continue;
        XII_TEST_BOOL(!views[view]->IsGpuSceneInitialized());
        const auto& instance = views[view]->GetRenderPipelineInstance();
        const auto* a        = static_cast<const xiiPipelineGPUReadback*>(instance.GetPass(0));
        const auto* b        = static_cast<const xiiPipelineGPUReadback*>(instance.GetPass(2));
        if (!XII_TEST_BOOL(a && b && a->m_pReadback && b->m_pReadback))
          continue;
        XII_TEST_BOOL(a->m_pReadback != b->m_pReadback);
        for (xiiUInt32 occurrence = 0; occurrence < 2; ++occurrence)
        {
          const auto*     readback = occurrence == 0 ? a : b;
          const xiiUInt32 seed     = occurrence == 0 ? 10 : 100;
          XII_TEST_INT(readback->m_uiObservedSeed, seed);
          XII_TEST_INT(readback->m_uiExecutedSeed, seed);
          xiiGALCommandListCreationDescription commands;
          commands.m_QueueFlags = xiiGALCommandQueueFlags::Graphics;
          auto mapping          = environment.GetDevice()->CreateCommandList(commands);
          mapping->Begin();
          void* data = nullptr;
          if (XII_TEST_RESULT(mapping->MapBuffer(readback->m_pReadback.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, data)))
          {
            const auto builds = view == 0 ? frame : frame == 3 ? 2U :
                                                                 1U;
            XII_TEST_INT(*static_cast<const xiiUInt32*>(data), seed + builds);
            XII_TEST_RESULT(mapping->UnmapBuffer(readback->m_pReadback.Borrow(), xiiGALMapType::Read));
          }
          mapping->End();
        }
      }
      if (frame > 1)
        XII_TEST_BOOL(views[0]->GetRenderGraph()->GetStatistics().m_bUsedCachedCompile);
    }
    const auto* firstA = static_cast<const xiiPipelineGPUReadback*>(views[0]->GetRenderPipelineInstance().GetPass(0));
    const auto* firstB = static_cast<const xiiPipelineGPUReadback*>(views[1]->GetRenderPipelineInstance().GetPass(0));
    XII_TEST_BOOL(firstA->m_pReadback != firstB->m_pReadback);
    for (auto* view : views)
      module->DestroyView(view->GetHandle());
  }
}

XII_CREATE_SIMPLE_TEST_GROUP(PipelineScheduling);
XII_CREATE_SIMPLE_TEST(PipelineScheduling, ReaderStateScheduling)
{
  for (bool exportAfterReaders : {false, true})
  {
    xiiRenderGraph graph;
    graph.BeginSetup(1);
    struct Texture
    {
      xiiRenderGraphTextureHandle m_Texture;
    };
    const auto producer = graph.AddPass<Texture>("Depth producer", xiiGALCommandQueueFlags::Graphics, [](Texture& data, xiiRenderGraphBuilder& builder) {
        xiiGALTextureCreationDescription texture;
        texture.m_Type = xiiGALResourceDimension::Texture2D;
        texture.m_Size = {8, 8};
        texture.m_Format = xiiGALResourceFormat::D32Float;
        texture.m_BindFlags = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
        data.m_Texture = builder.WriteTexture("Shared reader depth", texture, xiiGALResourceStateFlags::DepthWrite); }, [](const Texture&, xiiRenderGraphPassContext&) {});
    const auto read     = [&](xiiStringView name, xiiGALCommandQueueFlags::Enum queue, xiiGALResourceStateFlags::Enum state, bool exportResult = false) {
      graph.AddPass<Texture>(name, queue, [&](Texture& data, xiiRenderGraphBuilder& builder) {
        data.m_Texture = builder.ReadTexture(producer.first->m_Texture, state);
        if (exportResult)
          builder.ExportTexture(data.m_Texture, xiiGALResourceStateFlags::CopySource); }, [](const Texture&, xiiRenderGraphPassContext&) {}, true);
    };
    read("Establish sampled layout", xiiGALCommandQueueFlags::Compute, xiiGALResourceStateFlags::ShaderResource);
    read("Graphics reader", xiiGALCommandQueueFlags::Graphics, xiiGALResourceStateFlags::ShaderResource);
    read("Parallel compute reader", xiiGALCommandQueueFlags::Compute, xiiGALResourceStateFlags::ShaderResource, exportAfterReaders);
    if (!exportAfterReaders)
      read("Transition after readers", xiiGALCommandQueueFlags::Transfer, xiiGALResourceStateFlags::CopySource);
    graph.EndSetup();
    xiiRenderGraphCompileSettings settings;
    settings.m_bEnableSplitBarriers = true;
    if (!XII_TEST_RESULT(graph.Compile(settings)))
      return;
    const auto compiled    = graph.GetCompiledPasses();
    const auto submissions = graph.GetQueueSubmissions();
    const auto findPass    = [&](xiiUInt32 pass) -> const xiiRenderGraphCompiledPass* {
      for (const auto& entry : compiled)
        if (entry.m_uiPassIndex == pass)
          return &entry;
      return nullptr;
    };
    const auto findSubmission = [&](xiiUInt32 pass) {
      for (xiiUInt32 i = 0; i < submissions.GetCount(); ++i)
        for (xiiUInt32 index : submissions[i].m_PassOrder)
          if (compiled[index].m_uiPassIndex == pass)
            return i;
      return xiiInvalidIndex;
    };
    const auto* graphics = findPass(2);
    const auto* compute  = findPass(3);
    if (!XII_TEST_BOOL(graphics && compute))
      return;
    // Both readers use the original depth version, but the graphics reader must additionally
    // wait for the compute reader that performs its first sampled-layout transition.
    XII_TEST_BOOL(graphics->m_DependencyPassIndices.Contains(1));
    XII_TEST_BOOL(submissions[findSubmission(2)].m_WaitSubmissionIndices.Contains(findSubmission(1)));
    if (exportAfterReaders)
    {
      // The export transition runs after the final reader and must join the other reader too.
      XII_TEST_BOOL(compute->m_DependencyPassIndices.Contains(2));
      XII_TEST_BOOL(submissions[findSubmission(3)].m_WaitSubmissionIndices.Contains(findSubmission(2)));
    }
    else
    {
      // Readers in an already-established state remain independent across queues.
      XII_TEST_BOOL(!compute->m_DependencyPassIndices.Contains(2));
      XII_TEST_BOOL(!submissions[findSubmission(3)].m_WaitSubmissionIndices.Contains(findSubmission(2)));
      const auto* transition = findPass(4);
      if (!XII_TEST_BOOL(transition != nullptr))
        return;
      XII_TEST_BOOL(transition->m_DependencyPassIndices.Contains(2));
      XII_TEST_BOOL(transition->m_DependencyPassIndices.Contains(3));
      XII_TEST_BOOL(submissions[findSubmission(4)].m_WaitSubmissionIndices.Contains(findSubmission(2)));
      XII_TEST_BOOL(submissions[findSubmission(4)].m_WaitSubmissionIndices.Contains(findSubmission(3)));
    }
    xiiUInt32 transitionCount = 0;
    for (const auto& barrier : graph.GetBarriers())
    {
      if (barrier.m_BeforeState == xiiGALResourceStateFlags::ShaderResource && barrier.m_AfterState == xiiGALResourceStateFlags::CopySource)
      {
        ++transitionCount;
        XII_TEST_INT(barrier.m_TransitionType, xiiGALStateTransitionType::Immediate);
      }
    }
    XII_TEST_INT(transitionCount, 1);
  }
}

XII_CREATE_SIMPLE_TEST(PipelineScheduling, OffscreenRendering)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(
    environment.GetDevice()->WaitIdle();
    xiiStartup::ShutdownHighLevelSystems();
    xiiGALDevice::SetDefaultDevice(nullptr);
    environment.Shutdown(););

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Authored target and presentation resources render without scene services")
  {
    xiiGALTextureCreationDescription description;
    description.m_Type      = xiiGALResourceDimension::Texture2D;
    description.m_Size      = xiiSizeU32(32, 32);
    description.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    description.m_BindFlags = xiiGALBindFlags::RenderTarget;
    auto target             = environment.GetDevice()->CreateTexture(description);
    if (!XII_TEST_BOOL(target != nullptr))
      return;
    xiiWorldDescription worldDescription("Explicit Native Pipeline Test");
    xiiWorld            world(worldDescription);
    XII_LOCK(world.GetWriteMarker());
    auto*     module = world.GetOrCreateModule<xiiRenderWorldModule>();
    xiiCamera camera;
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 100.0f);
    xiiView*   view   = nullptr;
    const auto handle = module->CreateView("Explicit", view);
    view->SetCamera(&camera);
    view->SetViewport(xiiRectFloat(0, 0, 32, 32));
    view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget).Borrow());
    const auto                                 targets      = xiiLoadPipelineLibraryResource("SceneTargets");
    const auto                                 presentation = xiiLoadPipelineLibraryResource("ViewPresentation");
    xiiResourceLock<xiiRenderPipelineResource> targetSource(targets, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    xiiResourceLock<xiiRenderPipelineResource> outputSource(presentation, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!XII_TEST_BOOL(targetSource.IsValid() && outputSource.IsValid() && targetSource->GetAuthoring() && outputSource->GetAuthoring()))
      return;
    const auto targetDefinition = targetSource->GetAuthoring();
    const auto outputDefinition = outputSource->GetAuthoring();
    xiiUuid    clearNode;
    for (const auto& pass : targetDefinition->m_Descriptor.m_Passes)
      if (pass.m_sType == "xiiClearTexturePass")
        clearNode = pass.m_Id;
    if (!XII_TEST_BOOL(clearNode.IsValid()))
      return;
    xiiRenderPipelineResourceDescriptor definition;
    definition.m_References.PushBack({targetDefinition->m_Descriptor.m_Id, targets.GetResourceID()});
    definition.m_References.PushBack({outputDefinition->m_Descriptor.m_Id, presentation.GetResourceID()});
    const auto initialize = definition.AddSubgraph("Selected target resource", targetDefinition->m_Descriptor.m_Id);
    const auto present    = definition.AddSubgraph("Selected presentation resource", outputDefinition->m_Descriptor.m_Id);
    auto&      color      = definition.m_Passes[initialize].m_Overrides.ExpandAndGetRef();
    color.m_Path.PushBack(clearNode);
    color.m_Properties.Insert("Color", xiiColor(1, 0, 0));
    XII_TEST_RESULT(definition.Connect(initialize, "Color", present, "Color"));
    definition.m_Interface.PushBack({"ViewOutput", false, definition.m_Passes[present].m_Id, "ViewOutput"});
    xiiVariantDictionary display;
    display.Insert("PaperWhiteNits", 203.0f);
    definition.m_Parameters.Insert("DisplayOutput", display);
    const auto originalPaperWhite = view->GetDisplayOutputSettings().m_fPaperWhiteNits;
    view->SetRenderPipelineResource(xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Explicit native view pipeline", std::move(definition)));
    xiiGALTextureReadback readback(environment.GetDevice());
    for (xiiUInt32 frame = 0; frame < 2; ++frame)
    {
      if (frame == 1)
      {
        display.Insert("PaperWhiteNits", 300.0f);
        view->SetPipelineParameter("DisplayOutput", display);
      }
      world.Update();
      XII_TEST_FLOAT(view->GetDisplayOutputSettings().m_fPaperWhiteNits, originalPaperWhite, 0.0001f);
      environment.GetDevice()->WaitIdle();
      XII_TEST_INT(view->GetRenderGraph()->GetStatistics().m_uiCompiledPassCount, 2);
      XII_TEST_INT(view->GetRenderGraph()->GetStatistics().m_uiQueueSubmissionCount, 1);
      XII_TEST_BOOL(view->GetRenderPipelineInstance().GetPipeline()->GetExtractionRequirements().IsNoFlagSet());
      XII_TEST_BOOL(module->GetGpuInstanceData() == nullptr);
      XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
      const auto& execution = view->GetRenderPipelineInstance();
      const auto* output    = execution.GetInterfaceBinding(execution.GetPipeline()->FindInterface("ViewOutput", false));
      XII_TEST_BOOL(output && view->GetRenderGraph()->GetExportedTexture(output->m_hTexture) == target);
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
            {
              XII_TEST_INT(row[x * 4], 255);
              XII_TEST_INT(row[x * 4 + 1], 0);
              XII_TEST_INT(row[x * 4 + 2], 0);
              XII_TEST_INT(row[x * 4 + 3], 255);
            }
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
