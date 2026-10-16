/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/Passes/RayTracing/RayTracingScenePass.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>
#include <Shaders/Pipeline/Passes/RayTracing/RayTracingMaterialData.h>
#include <bit>

class xiiRaySceneBindingRelay : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRaySceneBindingRelay, xiiRenderPipelinePass);

public:
  xiiRenderPipelineAccelerationStructureInputPin  m_InputScene;
  xiiRenderPipelineAccelerationStructureOutputPin m_OutputScene{false, xiiRenderPipelinePinAccess::Read};
  xiiRenderPipelineBufferInputPin                 m_InputMaterials, m_InputGeometry;
  xiiRenderPipelineBufferOutputPin                m_OutputMaterials{false, {}, xiiRenderPipelinePinAccess::Read}, m_OutputGeometry{false, {}, xiiRenderPipelinePinAccess::Read};
  xiiResult                                       Build(xiiRenderPipelinePassContext& context) override
  {
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputScene, context.GetInputAccelerationStructure(m_InputScene)));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputMaterials, context.GetInputBuffer(m_InputMaterials)));
    return context.SetOutput(m_OutputGeometry, context.GetInputBuffer(m_InputGeometry));
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRaySceneBindingRelay, 1, xiiRTTIDefaultAllocator<xiiRaySceneBindingRelay>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("SceneInput", m_InputScene)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinNameAttribute("Scene")),
      XII_MEMBER_PROPERTY("SceneOutput", m_OutputScene)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinNameAttribute("Scene")),
      XII_MEMBER_PROPERTY("MaterialsInput", m_InputMaterials)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinNameAttribute("Materials")),
      XII_MEMBER_PROPERTY("MaterialsOutput", m_OutputMaterials)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinNameAttribute("Materials")),
      XII_MEMBER_PROPERTY("GeometryInput", m_InputGeometry)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinNameAttribute("Geometry")),
      XII_MEMBER_PROPERTY("GeometryOutput", m_OutputGeometry)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiRenderPipelinePinNameAttribute("Geometry")),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  struct Hit
  {
    xiiVec4    m_Surface;
    xiiVec4U32 m_Identity;
  };
  bool ReadHits(xiiGALDevice& device, xiiGALBuffer* source, Hit (&hits)[4])
  {
    if (!XII_TEST_BOOL(source != nullptr))
      return false;
    xiiGALBufferCreationDescription description;
    description.m_uiSize                              = sizeof(hits);
    description.m_Usage                               = xiiGALResourceUsage::Staging;
    description.m_CPUAccessFlags                      = xiiGALCPUAccessFlag::Read;
    auto                                 ownedStaging = source->GetDescription().m_Usage == xiiGALResourceUsage::Staging ? xiiSharedPtr<xiiGALBuffer>() : device.CreateBuffer(description);
    auto*                                staging      = ownedStaging ? ownedStaging.Borrow() : source;
    xiiGALCommandListCreationDescription commandDescription;
    commandDescription.m_QueueFlags = xiiGALCommandQueueFlags::Graphics;
    auto commands                   = device.CreateCommandList(commandDescription);
    if (!XII_TEST_BOOL(staging && commands && source))
      return false;
    if (ownedStaging)
    {
      commands->Begin();
      commands->CopyBufferRegion(source, 0, staging, 0, sizeof(hits));
      commands->End();
      auto queue = device.GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
      queue->WaitForFenceValue(queue->Submit(commands.Borrow()));
    }
    commands->Begin();
    void*      bytes  = nullptr;
    const auto mapped = commands->MapBuffer(staging, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, bytes);
    if (mapped.Succeeded() && bytes)
      xiiMemoryUtils::Copy(hits, static_cast<const Hit*>(bytes), 4);
    if (mapped.Succeeded())
      commands->UnmapBuffer(staging, xiiGALMapType::Read).AssertSuccess();
    commands->End();
    return XII_TEST_RESULT(mapped) && XII_TEST_BOOL(bytes != nullptr);
  }

  xiiMeshResourceDescriptor MakeMorphMesh(float deltaZ)
  {
    xiiMeshResourceDescriptor mesh;
    mesh.m_UsageFlags.Add(xiiMeshResourceUsageFlags::Skinned | xiiMeshResourceUsageFlags::MorphTargets);
    auto& buffer = mesh.GetMeshBufferDescriptor();
    buffer.AddCommonStreams();
    xiiMeshPackedVertex vertices[6];
    vertices[0].m_vPosition = xiiVec3(-0.25f, -0.25f, 0);
    vertices[1].m_vPosition = xiiVec3(0.25f, -0.25f, 0);
    vertices[2].m_vPosition = xiiVec3(0, 0.25f, 0);
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      vertices[i].m_vBoneIndices0 = xiiVec4U16(0, 65000, 65000, 65000);
      vertices[i].m_BoneWeights0  = xiiColorLinearUB(127, 0, 0, 0); // Normalize a non-255 weight sum.
      vertices[i + 3].m_vPosition = xiiVec3(0, 0, deltaZ);
      vertices[i + 3].m_vNormal   = xiiVec3(0, 0.5f, 0);
      vertices[i + 3].m_vTangent  = xiiVec4::MakeZero();
    }
    buffer.SetVertexData(xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(vertices), sizeof(vertices)), 6, sizeof(xiiMeshPackedVertex));
    const xiiUInt32 indices[] = {0, 1, 2};
    buffer.SetIndexData(xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(indices), sizeof(indices)), 3, xiiGALValueType::UInt32);
    mesh.AddSection(1, 0, 0);
    auto& target = mesh.m_MorphTargets.ExpandAndGetRef();
    target.m_Name.Assign("Raise");
    target.m_uiVertexOffset = 3;
    target.m_uiVertexCount  = 3;
    target.m_Bounds         = xiiBoundingBoxSphere::MakeFromBox(xiiBoundingBox::MakeFromMinMax(xiiVec3::MakeZero(), xiiVec3(0, 0, deltaZ)));
    mesh.ComputeBounds();
    return mesh;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(PipelineExtraction, RayTracingDeformation)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  if (!xiiRayTracingResources::IsHardwareRayTracingSupported())
  {
    xiiTestFramework::Output(xiiTestOutput::Details, "Hardware ray tracing unavailable: deformation trace test skipped.");
    return;
  }
  const auto mesh    = xiiResourceManager::LoadResource<xiiMeshResource>("RayTracingDeformationTest.Mesh");
  auto       publish = [&](float deltaZ) {
    auto                                      description = MakeMorphMesh(deltaZ);
    xiiUniquePtr<xiiResourceLoaderFromMemory> loader      = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
    xiiMemoryStreamWriter                     stream(&loader->m_CustomData);
    description.Save(stream);
    xiiResourceManager::UpdateResourceWithCustomLoader(mesh, std::move(loader));
    xiiResourceLock<xiiMeshResource> loaded(mesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    XII_TEST_BOOL(loaded.GetAcquireResult() == xiiResourceAcquireResult::Final);
    XII_TEST_INT(loaded->GetMorphTargets().GetCount(), 1);
  };
  publish(1);
  xiiWorldDescription description("World-owned deformed ray geometry");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto               module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiComponentHandle components[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    xiiGameObjectDescription objectDescription;
    objectDescription.m_bDynamic = true;
    xiiGameObject* object        = nullptr;
    world.CreateObject(objectDescription, object);
    xiiSkinnedMeshComponent* component = nullptr;
    components[i]                      = xiiSkinnedMeshComponent::CreateComponent(object, component);
    component->SetMesh(mesh);
    const auto pose = xiiMat4::MakeTranslation(xiiVec3(i == 0 ? -1.0f : 1.0f, 0, 0));
    XII_TEST_RESULT(component->SetSkinningMatrices(xiiMakeArrayPtr(&pose, 1)));
    const float weight = i == 0 ? 0.25f : 0.75f;
    component->SetMorphWeights(xiiMakeArrayPtr(&weight, 1));
  }
  world.Update();
  const auto                                    shader            = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/RayTracingDeformationTest.xiiShader");
  const auto                                    permutationHandle = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, {}, false);
  xiiResourceLock<xiiShaderPermutationResource> permutation(permutationHandle, xiiResourceAcquireMode::BlockTillLoaded);
  if (!XII_TEST_BOOL(permutation.IsValid() && permutation->IsShaderValid()))
    return;
  xiiGALRayTracingPipelineStateCreationDescription pipelineDescription;
  pipelineDescription.m_pPipelineResourceSignature               = permutation->GetPipelineResourceSignature();
  pipelineDescription.m_RayTracingPipeline.m_uiMaxRecursionDepth = 1;
  pipelineDescription.m_uiMaximumPayloadSize                     = sizeof(xiiVec4) + sizeof(xiiUInt32) + sizeof(float);
  pipelineDescription.m_uiMaximumAttributeSize                   = 2 * sizeof(float);
  auto& rayGeneration                                            = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  rayGeneration.m_sName.Assign("RayGen");
  rayGeneration.m_pShader = permutation->GetGALShader(xiiGALShaderType::RayGeneration);
  auto& miss              = pipelineDescription.m_GeneralShaders.ExpandAndGetRef();
  miss.m_sName.Assign("Miss");
  miss.m_pShader = permutation->GetGALShader(xiiGALShaderType::RayMiss);
  auto& closest  = pipelineDescription.m_TriangleHitShaders.ExpandAndGetRef();
  closest.m_sName.Assign("Hit");
  closest.m_pClosestHitShader = permutation->GetGALShader(xiiGALShaderType::RayClosestHit);
  const auto pipeline         = xiiGALPipelineCache::GetPipeline(pipelineDescription);
  if (!XII_TEST_BOOL(pipeline != nullptr))
    return;
  const auto&                     properties = environment.GetDevice()->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
  const xiiUInt64                 stride     = xiiMemoryUtils::AlignSize(xiiUInt64(properties.m_uiShaderGroupHandleSize), xiiUInt64(xiiMath::Max(1U, properties.m_uiShaderGroupBaseAlignment)));
  xiiGALBufferCreationDescription sbtDescription;
  sbtDescription.m_uiSize                 = stride * 3;
  sbtDescription.m_Mode                   = xiiGALBufferMode::Raw;
  sbtDescription.m_BindFlags              = xiiGALBindFlags::RayTracing;
  sbtDescription.m_Usage                  = xiiGALResourceUsage::Mutable;
  auto                                sbt = environment.GetDevice()->CreateBuffer(sbtDescription);
  xiiRenderPipelineResourceDescriptor rayLeaf;
  xiiVariantDictionary                rayProperties;
  rayProperties.Insert("UseViewSelection", false);
  rayProperties.Insert("RequireHardwareRayTracing", true);
  rayLeaf.AddPass("Ray scene", "xiiRayTracingScenePass", rayProperties);
  rayLeaf.AddPass("Typed scene consumer", "xiiRaySceneBindingRelay");
  XII_TEST_RESULT(rayLeaf.Connect(0, "Scene", 1, "Scene"));
  XII_TEST_RESULT(rayLeaf.Connect(0, "Materials", 1, "Materials"));
  XII_TEST_RESULT(rayLeaf.Connect(0, "Geometry", 1, "Geometry"));
  xiiGraphDefinition rayDefinition;
  XII_TEST_RESULT(rayLeaf.MakeGraphDefinition(rayDefinition));
  xiiRenderPipelineResourceDescriptor rayRoot;
  rayRoot.m_Definitions.PushBack(rayDefinition);
  rayRoot.AddSubgraph("First ray scene", rayDefinition.m_Id);
  rayRoot.AddSubgraph("Second ray scene", rayDefinition.m_Id);
  xiiSharedPtr<const xiiRenderPipeline> rayCompiled;
  if (!XII_TEST_RESULT(xiiRenderPipeline::Compile(rayRoot, rayCompiled)))
    return;
  xiiRenderPipelineInstance rayScenes;
  if (!XII_TEST_RESULT(rayScenes.Initialize(rayCompiled)))
    return;
  xiiRenderGraphContext context;
  XII_TEST_RESULT(context.Initialize("Skinned and morphed ray trace regression"));
  xiiUInt32 ids[2] = {};
  struct Capture
  {
    xiiSharedPtr<xiiGALBuffer> m_Buffers[2];
    xiiVec3                    m_Normals[2];
    xiiUInt32                  m_Ids[2];
    float                      m_Handedness[2], m_Distances[2];
    xiiUInt32                  m_FirstRay;
  };
  xiiDynamicArray<Capture> pending;
  auto                     check = [&](const Capture& capture) {
    for (const auto& buffer : capture.m_Buffers)
    {
      Hit hits[4];
      if (!ReadHits(*environment.GetDevice(), buffer.Borrow(), hits))
        return;
      for (xiiUInt32 ray = 0; ray < 4; ++ray)
      {
        const bool first = ray == capture.m_FirstRay, second = ray == 2;
        const auto owner = first ? 0U : 1U;
        XII_TEST_INT(hits[ray].m_Identity.x, first || second ? capture.m_Ids[owner] : 0);
        XII_TEST_FLOAT(hits[ray].m_Surface.x, first || second ? capture.m_Distances[owner] : -1.0f, 0.001f);
        if (first || second)
        {
          XII_TEST_VEC3(xiiVec3(hits[ray].m_Surface.y, hits[ray].m_Surface.z, hits[ray].m_Surface.w), capture.m_Normals[owner], 0.001f);
          XII_TEST_FLOAT(std::bit_cast<float>(hits[ray].m_Identity.y), capture.m_Handedness[owner], 0);
        }
      }
    }
  };
  auto render = [&](xiiUInt32 expectedDispatches, float distance0, float distance1, xiiUInt32 firstRay, bool defer = false) {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    xiiExtractedRenderData  packets;
    xiiMsgExtractRenderData request;
    request.m_pExtractedRenderData = &packets;
    for (const auto handle : components)
    {
      const xiiComponent* component = nullptr;
      if (static_cast<const xiiWorld&>(world).TryGetComponent(handle, component))
        component->SendMessage(request);
    }
    packets.SortAndBatches();
    if (!XII_TEST_RESULT(module->UpdateGpuInstances(packets, frame.GetFrameIndex())))
      return;
    const auto data = module->GetGpuInstanceData();
    if (!XII_TEST_INT(data->GetInstanceCount(), 2))
      return;
    xiiVec3 normals[2];
    float   handedness[2] = {1, 1};
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      const auto&          record = data->GetInstances()[i];
      xiiGpuInstanceSource source;
      XII_TEST_RESULT(data->Resolve(record.m_uiObjectId, source));
      const auto owner   = source.m_hOwnerComponent == components[0] ? 0U : 1U;
      ids[owner]         = record.m_uiObjectId;
      const auto weights = data->GetMorphWeights(i);
      auto       normal  = xiiVec3(0, weights.IsEmpty() ? 0.0f : weights[0] * 0.5f, 1);
      if (record.m_uiSkinningCount != 0)
      {
        auto linear       = data->GetSkinningMatrices()[record.m_uiSkinningOffset].GetRotationalPart();
        handedness[owner] = linear.GetDeterminant() < 0 ? -1.0f : 1.0f;
        if (linear.Invert().Succeeded())
        {
          linear.Transpose();
          normal = linear * normal;
        }
      }
      normals[owner] = normal.GetNormalized();
    }
    auto& graph = context.GetGraph();
    auto& cache = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    xiiView*   buildView       = nullptr;
    const auto buildViewHandle = module->CreateView("Explicit ray scene build", buildView);
    XII_SCOPE_EXIT(module->DestroyView(buildViewHandle));
    const auto builtScenes = rayScenes.Build(buildView, graph, context.GetBlackboard());
    if (!XII_TEST_RESULT(builtScenes))
    {
      xiiTestFramework::Output(xiiTestOutput::Details, "Ray scene build: %s", rayScenes.GetLastBuildError().GetStartPointer());
      graph.EndSetup();
      return;
    }
    const auto inspect = [&](xiiUInt32 index, xiiUInt32 expected) {
      xiiRayTracingResources::BuildHandles result;
      const auto*                          structure = rayScenes.GetOutputBinding(index + 1, "Scene");
      const auto*                          material  = rayScenes.GetOutputBinding(index + 1, "Materials");
      const auto*                          geometry  = rayScenes.GetOutputBinding(index + 1, "Geometry");
      if (!XII_TEST_BOOL(structure && material && geometry))
        return result;
      result.m_pTopLevelAS      = structure->m_AccelerationStructure.m_pStructure;
      result.m_hSceneDependency = structure->m_AccelerationStructure.m_Dependency;
      result.m_hMaterialData    = material->m_hBuffer;
      result.m_hGeometryData    = geometry->m_hBuffer;
      const auto* producer      = xiiDynamicCast<const xiiRayTracingScenePass*>(rayScenes.GetPass(index));
      if (XII_TEST_BOOL(producer != nullptr))
      {
        const auto& statistics = producer->GetLastBuildStatistics();
        XII_TEST_INT(statistics.m_uiInstanceCount, 2);
        XII_TEST_INT(statistics.m_uiDeformationDispatchCount, expected);
        XII_TEST_INT(statistics.m_uiDynamicBLASBuildCount, expected);
      }
      return result;
    };
    const auto first  = inspect(0, expectedDispatches);
    const auto second = inspect(2, 0);
    if (!XII_TEST_BOOL(first.m_pTopLevelAS && second.m_pTopLevelAS))
    {
      graph.EndSetup();
      return;
    }
    XII_TEST_BOOL(first.m_pTopLevelAS != second.m_pTopLevelAS);
    struct Trace
    {
      xiiRayTracingResources::BuildHandles m_Build;
      xiiRenderGraphBufferHandle           m_Output, m_Sbt;
    };
    xiiRenderGraphBufferHandle outputs[2];
    Capture                    capture;
    capture.m_FirstRay     = firstRay;
    capture.m_Distances[0] = distance0;
    capture.m_Distances[1] = distance1;
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      capture.m_Ids[i]        = ids[i];
      capture.m_Normals[i]    = normals[i];
      capture.m_Handedness[i] = handedness[i];
    }
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      auto pass     = graph.AddPass<Trace>("Deformed geometry trace", xiiGALCommandQueueFlags::Graphics, [&](Trace& trace, xiiRenderGraphBuilder& builder) {
        trace.m_Build = view == 0 ? first : second;
        if (trace.m_Build.m_hSceneDependency.IsValid()) trace.m_Build.m_hSceneDependency = builder.ReadBuffer(trace.m_Build.m_hSceneDependency, xiiGALResourceStateFlags::BuildASRead);
        trace.m_Build.m_hMaterialData = builder.ReadBuffer(trace.m_Build.m_hMaterialData, xiiGALResourceStateFlags::ShaderResource);
        trace.m_Build.m_hGeometryData = builder.ReadBuffer(trace.m_Build.m_hGeometryData, xiiGALResourceStateFlags::ShaderResource);
        trace.m_Sbt = builder.ImportBuffer("Deformation Test SBT", sbt, sbt->GetResourceState());
        trace.m_Sbt = builder.WriteBuffer(trace.m_Sbt, xiiGALResourceStateFlags::RayTracing);
        xiiGALBufferCreationDescription output;
        output.m_uiSize = 4 * sizeof(Hit); output.m_uiElementByteStride = sizeof(Hit);
        output.m_Mode = xiiGALBufferMode::Structured; output.m_BindFlags = xiiGALBindFlags::UnorderedAccess;
        output.m_Usage = xiiGALResourceUsage::Mutable;
        xiiStringBuilder name; name.SetFormat("Deformation Test Results {}", view);
        trace.m_Output = builder.WriteBuffer(name, output, xiiGALResourceStateFlags::UnorderedAccess);
        builder.ExportBuffer(trace.m_Output, xiiGALResourceStateFlags::CopySource); }, [pipeline, stride](const Trace& trace, xiiRenderGraphPassContext& context) {
        auto& commands = context.GetCommandList();
        commands.SetPipelineState(pipeline.Borrow());
        commands.ResolveAndSetAccelerationStructure("g_Scene", trace.m_Build.m_pTopLevelAS.Borrow(), xiiGALShaderType::RayGeneration);
        commands.ResolveAndSetUnorderedAccessBufferView("g_Results", context.GetBuffer(trace.m_Output)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
        commands.ResolveAndSetShaderResourceBufferView("g_Geometry", context.GetBuffer(trace.m_Build.m_hGeometryData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
        commands.ResolveAndSetShaderResourceBufferView("g_Materials", context.GetBuffer(trace.m_Build.m_hMaterialData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
        xiiGALBindlessResourceTable::BindBufferSRVs(commands, "g_RayTracingBuffers", xiiGALShaderType::RayClosestHit);
        commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
        xiiGALUpdateSBTDescription update;
        update.m_pPipelineState = pipeline.Borrow(); update.m_pShaderBindingTable = context.GetBuffer(trace.m_Sbt);
        update.m_RayGenerationTable = {0, stride, stride}; update.m_MissTable = {stride, stride, stride}; update.m_HitTable = {stride * 2, stride, stride};
        commands.UpdateSBT(update);
        xiiGALTraceRaysDescription rays(update.m_pShaderBindingTable, 4, 1, 1);
        rays.m_RayGenerationTable = update.m_RayGenerationTable; rays.m_MissTable = update.m_MissTable; rays.m_HitTable = update.m_HitTable;
        commands.TraceRays(rays); });
      outputs[view] = pass.first->m_Output;
      if (defer)
      {
        xiiGALBufferCreationDescription staging;
        staging.m_uiSize         = 4 * sizeof(Hit);
        staging.m_Usage          = xiiGALResourceUsage::Staging;
        staging.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
        capture.m_Buffers[view]  = environment.GetDevice()->CreateBuffer(staging);
        struct Copy
        {
          XII_DECLARE_POD_TYPE();
          xiiRenderGraphBufferHandle m_Source, m_Destination;
        };
        graph.AddPass<Copy>("Delayed deformation capture", xiiGALCommandQueueFlags::Graphics, [&](Copy& copy, xiiRenderGraphBuilder& builder) {
          copy.m_Source = builder.ReadBuffer(outputs[view], xiiGALResourceStateFlags::CopySource);
          xiiStringBuilder name; name.SetFormat("Delayed Ray Capture {} {}", frame.GetFrameIndex(), view);
          copy.m_Destination = builder.WriteBuffer(builder.ImportBuffer(name, capture.m_Buffers[view], capture.m_Buffers[view]->GetResourceState()), xiiGALResourceStateFlags::CopyDestination);
          builder.SetPassSideEffects(true); }, [](const Copy& copy, xiiRenderGraphPassContext& context) { context.GetCommandList().CopyBufferRegion(context.GetBuffer(copy.m_Source), 0, context.GetBuffer(copy.m_Destination), 0, 4 * sizeof(Hit)); });
      }
    }
    graph.EndSetup();
    if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache)))
      return;
    frame.End();
    if (defer)
    {
      pending.PushBack(std::move(capture));
      return;
    }
    environment.GetDevice()->WaitIdle();
    for (xiiUInt32 i = 0; i < 2; ++i)
      capture.m_Buffers[i] = graph.GetExportedBuffer(outputs[i]);
    check(capture);
  };
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Morph before skinning, independent poses, shared work and warmed frame slots")
  {
    for (xiiUInt32 i = 0; i < xiiRayTracingResources::GetConfiguration().m_uiFramesInFlight; ++i)
      render(2, 9.75f, 9.25f, 0);
    render(0, 9.75f, 9.25f, 0);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Skin/morph changes update BLAS and TLAS bounds without moving the world object")
  {
    xiiSkinnedMeshComponent* component = nullptr;
    if (!XII_TEST_BOOL(world.TryGetComponent(components[0], component)))
      return;
    const auto pose = xiiMat4::MakeTranslation(xiiVec3(2, 0, 0)) * xiiMat4::MakeScaling(xiiVec3(-1, 2, 1));
    XII_TEST_RESULT(component->SetSkinningMatrices(xiiMakeArrayPtr(&pose, 1)));
    const float weight = -0.5f;
    component->SetMorphWeights(xiiMakeArrayPtr(&weight, 1));
    world.Update();
    render(1, 10.5f, 9.25f, 3);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Bind-pose restoration and resource reload replace obsolete derived geometry")
  {
    xiiSkinnedMeshComponent* component = nullptr;
    if (!XII_TEST_BOOL(world.TryGetComponent(components[0], component)))
      return;
    XII_TEST_RESULT(component->SetSkinningMatrices({}));
    component->SetMorphWeights({});
    world.Update();
    render(0, 10, 9.25f, 1);
    publish(2);
    world.Update();
    render(1, 10, 8.5f, 1);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Delayed captures survive frame-slot reuse and authoritative component removal")
  {
    const auto frames = xiiRayTracingResources::GetConfiguration().m_uiFramesInFlight;
    for (xiiUInt32 i = 0; i < frames * 2; ++i)
    {
      xiiSkinnedMeshComponent* component = nullptr;
      if (!XII_TEST_BOOL(world.TryGetComponent(components[0], component)))
        return;
      const auto pose = xiiMat4::MakeTranslation(xiiVec3(i % 2 == 0 ? -1.0f : 2.0f, 0, 0));
      XII_TEST_RESULT(component->SetSkinningMatrices(xiiMakeArrayPtr(&pose, 1)));
      const float weight = i % 2 == 0 ? 0.25f : -0.5f;
      component->SetMorphWeights(xiiMakeArrayPtr(&weight, 1));
      world.Update();
      render(i < frames - 1 ? 2 : 1, i % 2 == 0 ? 9.5f : 11.0f, 8.5f, i % 2 == 0 ? 0 : 3, true);
    }
    xiiSkinnedMeshComponent* removed = nullptr;
    if (!XII_TEST_BOOL(world.TryGetComponent(components[0], removed)))
      return;
    const auto retiredId = ids[0];
    world.DeleteObjectNow(removed->GetOwner()->GetHandle());
    xiiGpuInstanceSource source;
    XII_TEST_BOOL(module->GetGpuInstanceData()->Resolve(retiredId, source).Failed());
    xiiGameObjectDescription authored;
    authored.m_bDynamic   = true;
    xiiGameObject* object = nullptr;
    world.CreateObject(authored, object);
    xiiSkinnedMeshComponent* replacement = nullptr;
    components[0]                        = xiiSkinnedMeshComponent::CreateComponent(object, replacement);
    replacement->SetMesh(mesh);
    world.Update();
    render(0, 10, 8.5f, 1);
    XII_TEST_BOOL(ids[0] != retiredId);
    environment.GetDevice()->WaitIdle();
    for (const auto& capture : pending)
      check(capture);
    pending.Clear();
    XII_TEST_BOOL(module->GetGpuInstanceData()->Resolve(retiredId, source).Failed());
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid morph weights fail without replacing the last valid extraction")
  {
    const xiiComponent* component = nullptr;
    if (!XII_TEST_BOOL(static_cast<const xiiWorld&>(world).TryGetComponent(components[1], component)))
      return;
    xiiExtractedRenderData  packets;
    xiiMsgExtractRenderData request;
    request.m_pExtractedRenderData = &packets;
    component->SendMessage(request);
    packets.SortAndBatches();
    if (!XII_TEST_INT(packets.GetAllRenderData().GetCount(), 1))
      return;
    auto packet                = *static_cast<const xiiMeshRenderData*>(packets.GetAllRenderData()[0]);
    packet.m_MorphWeights[0]   = xiiMath::NaN<float>();
    xiiRenderData* malformed[] = {&packet};
    XII_TEST_BOOL(module->UpdateGpuInstances(xiiMakeArrayPtr(malformed), 999999).Failed());
    XII_TEST_INT(module->GetGpuInstanceData()->GetInstanceCount(), 2);
    packet.m_MorphWeights.SetCount(2); // The mesh contains only one target.
    packet.m_MorphWeights[0] = packet.m_MorphWeights[1] = 0;
    XII_TEST_BOOL(module->UpdateGpuInstances(xiiMakeArrayPtr(malformed), 999999).Failed());
    for (const auto id : ids)
    {
      xiiGpuInstanceSource source;
      XII_TEST_RESULT(module->GetGpuInstanceData()->Resolve(id, source));
    }
  }
}
