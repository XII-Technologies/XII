/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Math/BoundingBox.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGIProbePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/DDGISceneUpdatePass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Materials/GpuSurfaceMaterial.h>
#include <Shaders/Pipeline/Passes/DDGI/DDGIWorldTraceConstants.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDDGISceneUpdatePass, 1, xiiRTTIDefaultAllocator<xiiDDGISceneUpdatePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("AllowAbsentSchedule", m_bAllowAbsentSchedule)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("UseWorldGeometry", m_bUseWorldGeometry)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("Schedule", m_Schedule)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ProbeConstants", m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera", m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ProbeUpdates", m_Updates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SceneInstances", m_Instances)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("GeometryMetadata", m_Geometry)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MeshletMetadata", m_Meshlets)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SurfaceMaterials", m_Materials)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitialProbeStates", m_StatesInput)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SceneDepth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("GBufferNormal", m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SourceRadiance", m_Radiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitialIrradiance", m_IrradianceInput)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitialDistance", m_DistanceInput)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ProbeStates", m_States)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Irradiance", m_Irradiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Distance", m_Distance)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiDDGISceneUpdatePass::Build(xiiRenderPipelinePassContext& context)
{
  if (m_bAllowAbsentSchedule && !context.GetInputData(m_Schedule))
    return XII_SUCCESS;
  const auto snapshot = xiiDDGIProbePassUtils::Snapshot(context, m_Schedule);
  const auto device   = xiiGALDevice::GetDefaultDevice();
  if (!snapshot || !snapshot->m_bDirectional || !device)
    return context.ReportFailure("DDGI scene updates require a directional probe schedule and a device.");
  const auto  depth = context.GetInputTexture(m_Depth), normal = context.GetInputTexture(m_Normal), radiance = context.GetInputTexture(m_Radiance);
  const auto* scene  = xiiScreenSpacePassUtils::Image(context, depth, 1);
  const auto  camera = context.GetInputBuffer(m_Camera), constants = context.GetInputBuffer(m_Constants);
  if (!scene || !xiiScreenSpacePassUtils::Image(context, normal, 2, &scene->m_Size) || !xiiScreenSpacePassUtils::Image(context, radiance, 3, &scene->m_Size) ||
      !xiiScreenSpacePassUtils::Uniform(context, camera, sizeof(xiiFrameCameraConstants)) || !xiiScreenSpacePassUtils::Uniform(context, constants, sizeof(xiiDDGIProbeConstants)))
    return context.ReportFailure("DDGI scene updates require matching depth, normal, radiance, and camera resources.");
  const auto  updates = context.GetInputBuffer(m_Updates), states = context.GetInputBuffer(m_StatesInput);
  const auto  irradiance = context.GetInputTexture(m_IrradianceInput), distance = context.GetInputTexture(m_DistanceInput);
  const auto* irradianceDescription = context.m_Graph.GetTextureDescription(irradiance);
  const auto* distanceDescription   = context.m_Graph.GetTextureDescription(distance);
  if (!xiiDDGIProbePassUtils::Buffer(context, updates, sizeof(xiiGpuDDGIProbeUpdate), snapshot->m_Updates.GetCount(), xiiGALBindFlags::ShaderResource) ||
      !xiiDDGIProbePassUtils::Buffer(context, states, sizeof(xiiGpuDDGIProbeState), snapshot->m_Probes.GetCount(), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess) ||
      !irradianceDescription || !distanceDescription || irradianceDescription->m_Size != snapshot->m_AtlasSize || distanceDescription->m_Size != snapshot->m_AtlasSize ||
      irradiance.GetResourceId() == distance.GetResourceId())
    return context.ReportFailure("DDGI scene update resources must match the directional probe schedule.");
  const auto& limits = device->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  const auto  groups = snapshot->m_Updates.GetCount();
  if (groups > limits.m_uiMaxThreadGroupCountX || limits.m_uiMaxThreadGroupInvocations < 64 || limits.m_uiMaxThreadGroupSizeX < 64)
    return context.ReportFailure("DDGI scene update budget exceeds compute dispatch limits.");
  xiiHashSet<xiiUInt32> seen;
  for (const auto& update : snapshot->m_Updates)
  {
    if (update.m_vMetadata.x >= snapshot->m_Probes.GetCount() || seen.Contains(update.m_vMetadata.x))
      return context.ReportFailure("DDGI scene update slots must be unique and within the probe domain.");
    seen.Insert(update.m_vMetadata.x);
  }
  const auto                 instances = context.GetInputBuffer(m_Instances), geometry = context.GetInputBuffer(m_Geometry), materials = context.GetInputBuffer(m_Materials);
  const auto*                world    = context.m_pView ? context.m_pView->GetGpuInstanceData() : nullptr;
  const auto&                features = device->GetFeatures();
  xiiDynamicArray<xiiUInt32> candidates;
  if (m_bUseWorldGeometry && world && instances.IsValid() && geometry.IsValid() && materials.IsValid() &&
      features.m_BindlessResources == xiiGALDeviceFeatureState::Enabled && features.m_ShaderResourceRuntimeArray == xiiGALDeviceFeatureState::Enabled)
  {
    auto coverage = xiiBoundingBox::MakeInvalid();
    for (const auto& update : snapshot->m_Updates)
      coverage.ExpandToInclude(update.m_vPositionAndHistoryWeight.GetAsVec3());
    coverage.Grow(xiiVec3(snapshot->m_Constants.SpacingHysteresisDistance.z));
    constexpr xiiUInt32 maxCandidates = 256;
    for (const auto index : context.m_pView->GetGpuInstanceSelection())
    {
      if (index >= world->GetInstanceCount())
        continue;
      const auto& instance = world->GetInstances()[index];
      const auto  lod      = instance.m_uiForcedLod == xiiInvalidIndex ? 0U : instance.m_uiForcedLod;
      if ((instance.m_uiFlags & xiiGpuInstanceFlags::Enabled) == 0 || (instance.m_uiFlags & xiiGpuInstanceFlags::Transparent) != 0 ||
          (instance.m_uiDrawLod != xiiInvalidIndex && instance.m_uiDrawLod != lod))
        continue;
      const auto center  = instance.m_BoundsCenterRadius.GetAsVec3();
      const auto extents = instance.m_BoundsExtents.GetAsVec3();
      if (!coverage.Overlaps(xiiBoundingBox::MakeFromMinMax(center - extents, center + extents)))
        continue;
      if (candidates.GetCount() == maxCandidates)
      {
        candidates.Clear();
        break;
      }
      // Keep unsupported deforming meshes as conservative blockers. An intersecting
      // ray rejects the software update instead of tracing through their bind pose.
      const bool deformed = instance.m_uiSkinningCount != 0 || !world->GetMorphWeights(index).IsEmpty();
      candidates.PushBack(index | (deformed ? 0x80000000U : 0U));
    }
  }
  const bool useWorld    = !candidates.IsEmpty();
  const auto meshlets    = context.GetInputBuffer(m_Meshlets);
  const bool useMeshlets = useWorld && meshlets.IsValid();
  if (useMeshlets)
  {
    const auto* description = context.m_Graph.GetBufferDescription(meshlets);
    if (!description || description->m_Mode != xiiGALBufferMode::Structured || description->m_uiElementByteStride != sizeof(xiiMeshlet) ||
        description->m_uiSize % sizeof(xiiMeshlet) != 0 || !description->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
      return context.ReportFailure("World probe tracing requires canonical structured meshlet metadata.");
  }
  auto&      pipeline = useMeshlets ? m_WorldMeshletPipeline : (useWorld ? m_WorldPipeline : m_Pipeline);
  const auto shader   = useMeshlets ? "Shaders/Pipeline/Passes/GlobalIllumination/DDGIWorldMeshletUpdate.xiiShader" :
                                      (useWorld ? "Shaders/Pipeline/Passes/GlobalIllumination/DDGIWorldUpdate.xiiShader" : "Shaders/Pipeline/Passes/GlobalIllumination/DDGISceneUpdate.xiiShader");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(pipeline, shader));
  xiiRenderPassResourceScope scope(context);
  xiiRenderGraphBufferHandle candidateBuffer, worldConstants;
  if (useWorld)
  {
    const auto frame = context.m_Graph.GetFrameIndex();
    if (!m_ProbeSampler)
    {
      xiiGALSamplerCreationDescription sampler;
      sampler.m_MinFilter = xiiGALFilterType::Linear;
      sampler.m_MagFilter = xiiGALFilterType::Linear;
      sampler.m_MipFilter = xiiGALFilterType::Linear;
      sampler.m_AddressU  = xiiGALTextureAddressMode::Wrap;
      sampler.m_AddressV  = xiiGALTextureAddressMode::Wrap;
      sampler.m_AddressW  = xiiGALTextureAddressMode::Wrap;
      m_ProbeSampler      = device->CreateSampler(sampler);
      if (!m_ProbeSampler)
        return context.ReportFailure("Could not create the world probe material sampler.");
    }
    struct Upload
    {
      xiiDynamicArray<xiiUInt32> m_Candidates;
      xiiDDGIWorldTraceConstants m_Layout;
      xiiRenderGraphBufferHandle m_CandidateBuffer, m_Constants;
    };
    const auto upload = context.m_Graph.AddPass<Upload>("Upload World Probe Candidates", xiiGALCommandQueueFlags::Transfer, [&](Upload& data, xiiRenderGraphBuilder& builder) {
      data.m_Candidates = std::move(candidates);
      data.m_Layout.ProbeSceneLayout = {xiiGeometryResidencyManager::GetMetadataBaseIndex(frame), xiiMaterialManager::GetGpuStorage().GetSurfaceBaseIndex(frame),
                                      sizeof(xiiMeshPackedVertex), offsetof(xiiMeshPackedVertex, m_vTexCoord0)};
      xiiGALBufferCreationDescription desc;
      desc.m_Mode = xiiGALBufferMode::Structured;
      desc.m_uiElementByteStride = sizeof(xiiUInt32);
      desc.m_uiSize = data.m_Candidates.GetCount() * sizeof(xiiUInt32);
      desc.m_BindFlags = xiiGALBindFlags::ShaderResource;
      data.m_CandidateBuffer = builder.WriteBuffer("World Probe Candidates", desc, xiiGALResourceStateFlags::CopyDestination);
      data.m_Constants = xiiRenderPassUtils::Constants(builder, "World Probe Layout", sizeof(xiiDDGIWorldTraceConstants)); }, [](const Upload& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      commands.UpdateBuffer(execute.GetBuffer(data.m_CandidateBuffer), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Candidates.GetData()), data.m_Candidates.GetCount() * sizeof(xiiUInt32)));
      auto* buffer = execute.GetBuffer(data.m_Constants);
      {
        xiiGALMapHelper<xiiDDGIWorldTraceConstants> mapped(commands, buffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *mapped = data.m_Layout;
      }
      xiiGALStateTransitionDescription transition;
      transition.m_pResource = buffer;
      transition.m_NewState = xiiGALResourceStateFlags::ConstantBuffer;
      transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
      commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1)); });
    candidateBuffer   = upload.first->m_CandidateBuffer;
    worldConstants    = upload.first->m_Constants;
  }
  struct Data
  {
    xiiRenderGraphBufferHandle                m_Camera, m_Constants, m_Updates, m_States;
    xiiRenderGraphBufferHandle                m_Instances, m_Geometry, m_Meshlets, m_Materials, m_Candidates, m_WorldConstants;
    xiiRenderGraphTextureHandle               m_Depth, m_Normal, m_Radiance, m_Irradiance, m_Distance;
    xiiSharedPtr<xiiGALComputePipelineState>  m_Pipeline;
    xiiSharedPtr<xiiGALSampler>               m_Sampler;
    xiiSharedPtr<const xiiDDGIProbeFrameData> m_Snapshot;
    xiiUInt32                                 m_uiGroups;
  };
  const auto setup = [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Camera     = builder.ReadBuffer(camera, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Constants  = builder.ReadBuffer(constants, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Updates    = builder.ReadBuffer(updates, xiiGALResourceStateFlags::ShaderResource);
    data.m_States     = builder.ReadWriteBuffer(states, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Depth      = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Normal     = builder.ReadTexture(normal, xiiGALResourceStateFlags::ShaderResource);
    data.m_Radiance   = builder.ReadTexture(radiance, xiiGALResourceStateFlags::ShaderResource);
    data.m_Irradiance = builder.ReadWriteTexture(irradiance, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Distance   = builder.ReadWriteTexture(distance, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Snapshot   = snapshot;
    data.m_Pipeline   = pipeline;
    data.m_uiGroups   = groups;
    if (useMeshlets)
      data.m_Meshlets = builder.ReadBuffer(meshlets, xiiGALResourceStateFlags::ShaderResource);
    if (useWorld)
    {
      data.m_Instances      = builder.ReadBuffer(instances, xiiGALResourceStateFlags::ShaderResource);
      data.m_Geometry       = builder.ReadBuffer(geometry, xiiGALResourceStateFlags::ShaderResource);
      data.m_Materials      = builder.ReadBuffer(materials, xiiGALResourceStateFlags::ShaderResource);
      data.m_Candidates     = builder.ReadBuffer(candidateBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.m_WorldConstants = builder.ReadBuffer(worldConstants, xiiGALResourceStateFlags::ConstantBuffer);
      data.m_Sampler        = m_ProbeSampler;
    }
  };
  const auto record = [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiSurfaceCameraConstants", execute.GetBuffer(data.m_Camera), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiDDGIProbeConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_ProbeUpdates", execute.GetBuffer(data.m_Updates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_ProbeStates", execute.GetBuffer(data.m_States)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Depth", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Normal", execute.GetTexture(data.m_Normal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Radiance", execute.GetTexture(data.m_Radiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Irradiance", execute.GetTexture(data.m_Irradiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Distance", execute.GetTexture(data.m_Distance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (data.m_WorldConstants.IsValid())
    {
      const auto bind = [&](xiiTempHashedString name, xiiRenderGraphBufferHandle handle) {
        commands.ResolveAndSetShaderResourceBufferView(name, execute.GetBuffer(handle)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      };
      bind("g_ProbeInstances", data.m_Instances);
      bind("g_ProbeGeometry", data.m_Geometry);
      if (data.m_Meshlets.IsValid())
        bind("g_Meshlets", data.m_Meshlets);
      bind("g_ProbeMaterials", data.m_Materials);
      bind("g_ProbeCandidates", data.m_Candidates);
      commands.ResolveAndSetConstantBuffer("xiiDDGIWorldTraceConstants", execute.GetBuffer(data.m_WorldConstants), xiiGALShaderType::Compute);
      commands.ResolveAndSetSampler("g_ProbeSampler", data.m_Sampler.Borrow());
      xiiGALBindlessResourceTable::BindBufferSRVs(commands, "g_Buffers", xiiGALShaderType::Compute);
      xiiGALBindlessResourceTable::BindTextureSRVs(commands, "g_Textures", xiiGALShaderType::Compute);
      xiiGALBindlessResourceTable::BindSamplers(commands, "g_MaterialSamplers", xiiGALShaderType::Compute);
    }
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
    {
      execute.ReportFailure("Could not bind directional probe scene update resources.");
      return;
    }
    commands.DispatchCompute({data.m_uiGroups, 1, 1});
    data.m_Snapshot->CommitUpdates();
  };
  const auto name    = useMeshlets ? "Trace World Meshlets for Directional Probes" : (useWorld ? "Trace World Geometry for Directional Probes" : "Update Directional Scene Probes");
  const auto emitted = context.m_Graph.AddPass<Data>(name, xiiGALCommandQueueFlags::Compute, setup, record);
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_States, emitted.first->m_States));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Irradiance, emitted.first->m_Irradiance));
  return context.SetOutput(m_Distance, emitted.first->m_Distance);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_Implementation_DDGISceneUpdatePass);
