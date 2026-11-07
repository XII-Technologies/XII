/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/RayTracedIrradiancePass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterPassUtils.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TopLevelAS.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/GlobalIllumination/GIPathReservoirConstants.h>
#include <Shaders/Pipeline/Passes/GlobalIllumination/RayTracedIrradianceConstants.h>
#include <Shaders/Pipeline/Passes/RayTracing/RayTracingMaterialData.h>
#include <Shaders/Pipeline/Passes/ReSTIR/EmissiveLightData.h>
#include <Shaders/Pipeline/SkyLightData.h>

struct xiiRayTracedIrradiancePass::ReservoirState : public xiiRefCounted
{
  struct Slot : public xiiRefCounted
  {
    xiiSharedPtr<xiiGALBuffer> m_Buffer;
    xiiSharedPtr<xiiGALFence>  m_Ready;
    xiiSizeU32                 m_Extent;
    xiiUInt64                  m_uiFrame = 0, m_uiLastUsedFrame = 0, m_uiFenceValue = 0;
  };

  static constexpr xiiUInt32                       s_uiMaxSlots     = 4;
  static constexpr xiiUInt64                       s_uiBytesPerSlot = 12 * 1024 * 1024;
  xiiHybridArray<xiiSharedPtr<Slot>, s_uiMaxSlots> m_Slots;
  xiiSharedPtr<Slot>                               m_Last;
  xiiSharedPtr<xiiGALBuffer>                       m_Empty;
  xiiUInt64                                        m_uiRevision = 0;
  xiiMutex                                         m_Mutex;
};

xiiRayTracedIrradiancePass::xiiRayTracedIrradiancePass() { m_Irradiance.SetResolution({xiiRenderPipelineTextureResolutionMode::Input}); }
xiiRayTracedIrradiancePass::~xiiRayTracedIrradiancePass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRayTracedIrradiancePass, 1, xiiRTTIDefaultAllocator<xiiRayTracedIrradiancePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Depth", m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Normal", m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera", m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Scene", m_Scene)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Materials", m_Materials)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Geometry", m_Geometry)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Lights", m_Lights)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightCounts", m_LightCounts)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LightCandidates", m_LightCandidates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("IESProfiles", m_IESProfiles)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("EmissiveTriangles", m_EmissiveTriangles)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("EmissiveHeader", m_EmissiveHeader)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("TriangleAliases", m_TriangleAliases)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("BlockAliases", m_BlockAliases)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AliasHeader", m_AliasHeader)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ReceiverVelocity", m_ReceiverVelocity)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ReceiverObject", m_ReceiverObject)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ReceiverMaterial", m_ReceiverMaterial)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("TemporalCamera", m_TemporalCamera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("UsePathReservoirs", m_bUsePathReservoirs)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("ReflectionProbes", m_Environment)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("EnvironmentLightSamples", m_uiEnvironmentLightSamples)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 8U)),
    XII_MEMBER_PROPERTY("EmissiveLightSamples", m_uiEmissiveLightSamples)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 8U)),
    XII_MEMBER_PROPERTY("Irradiance", m_Irradiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("SampleCount", m_uiSampleCount)->AddAttributes(new xiiDefaultValueAttribute(8U), new xiiClampValueAttribute(1U, 32U)),
    XII_MEMBER_PROPERTY("MaximumHits", m_uiMaximumHits)->AddAttributes(new xiiDefaultValueAttribute(2U), new xiiClampValueAttribute(1U, 4U)),
    XII_MEMBER_PROPERTY("AnimateSamples",m_bAnimateSamples)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("SampleOffset", m_uiSampleOffset),
    XII_MEMBER_PROPERTY("AnalyticLightSamples", m_uiAnalyticLightSamples)->AddAttributes(new xiiDefaultValueAttribute(4U), new xiiClampValueAttribute(0U, 8U)),
    XII_MEMBER_PROPERTY("PowerWeightedLights", m_bPowerWeightedLights)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("RayLength", m_fRayLength)->AddAttributes(new xiiDefaultValueAttribute(20.0f)),
    XII_MEMBER_PROPERTY("SurfaceBias", m_fSurfaceBias)->AddAttributes(new xiiDefaultValueAttribute(0.02f)),
    XII_MEMBER_PROPERTY("AllowUnsupportedHardware", m_bAllowUnsupportedHardware)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiRayTracedIrradiancePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  InvalidateTemporalHistory();
  return m_uiSampleCount > 0 && m_uiSampleCount <= 32 && m_uiMaximumHits > 0 && m_uiMaximumHits <= 4 && m_uiAnalyticLightSamples <= 8 && m_uiEmissiveLightSamples <= 8 && m_uiEnvironmentLightSamples <= 8 && xiiMath::IsFinite(m_fRayLength) && m_fRayLength > 0.001f && m_fRayLength <= 100000 && xiiMath::IsFinite(m_fSurfaceBias) && m_fSurfaceBias >= 0 && m_fSurfaceBias <= 10 ? XII_SUCCESS : XII_FAILURE;
}
void xiiRayTracedIrradiancePass::InvalidateTemporalHistory()
{
  if (m_ReservoirState)
  {
    XII_LOCK(m_ReservoirState->m_Mutex);
    m_ReservoirState->m_Last.Clear();
    ++m_ReservoirState->m_uiRevision;
  }
}

xiiResult xiiRayTracedIrradiancePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto depth = context.GetInputTexture(m_Depth);
  if (!depth.IsValid())
  {
    InvalidateTemporalHistory();
    return XII_SUCCESS;
  }
  const auto  device = xiiGALDevice::GetDefaultDevice();
  const auto& limits = device->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties;
  if (device->GetFeatures().m_RayTracing != xiiGALDeviceFeatureState::Enabled || !limits.m_CapabilityFlags.IsSet(xiiGALRayTracingCapabilityFlags::StandaloneShaders) || !xiiGALBindlessResourceTable::IsInitialized())
    return m_bAllowUnsupportedHardware ? XII_SUCCESS : context.ReportFailure("Ray irradiance requires hardware ray pipelines and bindless material resources.");
  const auto* input  = xiiScreenSpacePassUtils::Image(context, depth, 1);
  const auto  normal = context.GetInputTexture(m_Normal);
  const auto  camera = context.GetInputBuffer(m_Camera);
  if (!input || !xiiScreenSpacePassUtils::Image(context, normal, 2, &input->m_Size) || !xiiScreenSpacePassUtils::Uniform(context, camera, sizeof(xiiFrameCameraConstants)))
    return context.ReportFailure("Ray irradiance requires matching depth/normal images and exact camera constants.");
  const auto size = input->m_Size;
  if (xiiUInt64(size.width) * size.height > limits.m_uiMaxRayGenThreads)
    return context.ReportFailure("Ray irradiance exceeds ray generation dispatch limits.");
  xiiSizeU32 outputSize;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, m_Irradiance, depth, outputSize));
  if (outputSize != size)
    return context.ReportFailure("Ray irradiance requires input-sized output.");
  const auto  lights = context.GetInputBuffer(m_Lights), counts = context.GetInputBuffer(m_LightCounts), candidates = context.GetInputBuffer(m_LightCandidates), profiles = context.GetInputBuffer(m_IESProfiles);
  const auto* records   = context.m_Graph.GetBufferDescription(lights);
  const auto* selection = context.m_Graph.GetBufferDescription(candidates);
  const auto* ies       = context.m_Graph.GetBufferDescription(profiles);
  using xiiLightClusterPassUtils::Buffer;
  if (!records || !records->m_uiSize || records->m_uiSize % sizeof(xiiGpuLightData) || records->m_uiSize / sizeof(xiiGpuLightData) > 1048576 ||
      !Buffer(context, lights, sizeof(xiiGpuLightData), records->m_uiSize / sizeof(xiiGpuLightData), xiiGALBindFlags::ShaderResource) ||
      !selection || !selection->m_uiSize || selection->m_uiSize % sizeof(xiiUInt32) || selection->m_uiSize / sizeof(xiiUInt32) > records->m_uiSize / sizeof(xiiGpuLightData) ||
      !Buffer(context, candidates, sizeof(xiiUInt32), selection->m_uiSize / sizeof(xiiUInt32), xiiGALBindFlags::ShaderResource) ||
      !ies || !ies->m_uiSize || (ies->m_uiSize != sizeof(float) && ies->m_uiSize % (2048 * sizeof(float))) ||
      !Buffer(context, profiles, sizeof(float), ies->m_uiSize / sizeof(float), xiiGALBindFlags::ShaderResource) ||
      !Buffer(context, counts, sizeof(xiiVec4U32), 1, xiiGALBindFlags::ShaderResource))
    return context.ReportFailure("Ray irradiance requires canonical light, count, view-candidate and IES records.");
  const auto                 scene = context.GetInputAccelerationStructure(m_Scene);
  xiiRenderPassResourceScope scope(context);
  if (scene.IsEmpty())
  {
    InvalidateTemporalHistory();
    const auto setup = [&](auto& image, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription desc;
      desc.m_Type      = xiiGALResourceDimension::Texture2D;
      desc.m_Size      = size;
      desc.m_Format    = xiiGALResourceFormat::RGBA32Float;
      desc.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
      image            = builder.WriteTexture("Empty scene irradiance", desc, xiiGALResourceStateFlags::RenderTarget);
    };
    const auto record = [](const auto& image, xiiRenderGraphPassContext& execute) {
      xiiRenderPassUtils::ClearRenderTargetPreservingState(execute.GetCommandList(), execute.GetTexture(image)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    };
    const auto cleared = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, setup, record);
    return context.SetOutput(m_Irradiance, *cleared.first);
  }
  const auto  materials = context.GetInputBuffer(m_Materials), geometry = context.GetInputBuffer(m_Geometry);
  const auto* m = context.m_Graph.GetBufferDescription(materials);
  const auto* g = context.m_Graph.GetBufferDescription(geometry);
  if (!scene.IsValid() || !context.m_Graph.GetBufferDescription(scene.m_Dependency) || !m || !g || m->m_Mode != xiiGALBufferMode::Structured || g->m_Mode != xiiGALBufferMode::Structured || m->m_uiElementByteStride != sizeof(xiiRayTracingMaterialData) || g->m_uiElementByteStride != sizeof(xiiRayTracingGeometryData) || m->m_uiSize / sizeof(xiiRayTracingMaterialData) != g->m_uiSize / sizeof(xiiRayTracingGeometryData) || m->m_uiSize == 0 || m->m_uiSize % sizeof(xiiRayTracingMaterialData) != 0 || g->m_uiSize % sizeof(xiiRayTracingGeometryData) != 0 || !m->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) || !g->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
    return context.ReportFailure("Ray irradiance requires a current TLAS dependency and matching canonical hit records.");
  const bool                             emissiveSampling = m_uiEmissiveLightSamples != 0;
  const xiiRenderPipelineBufferInputPin* emissivePins[]   = {&m_EmissiveTriangles, &m_EmissiveHeader, &m_TriangleAliases, &m_BlockAliases, &m_AliasHeader};
  xiiRenderGraphBufferHandle             emissiveTables[5];
  if (emissiveSampling)
  {
    for (xiiUInt32 index = 0; index < 5; ++index)
      emissiveTables[index] = context.GetInputBuffer(*emissivePins[index]);
    const auto* triangles = context.m_Graph.GetBufferDescription(emissiveTables[0]);
    if (!triangles || triangles->m_uiSize == 0 || triangles->m_uiSize % sizeof(xiiGpuEmissiveTriangle) != 0 || triangles->m_uiSize / sizeof(xiiGpuEmissiveTriangle) > 16U * 1024U * 1024U)
      return context.ReportFailure("Ray irradiance requires bounded native emissive triangle records.");
    const xiiUInt32 capacity = xiiUInt32(triangles->m_uiSize / sizeof(xiiGpuEmissiveTriangle));
    if (!Buffer(context, emissiveTables[0], sizeof(xiiGpuEmissiveTriangle), capacity, xiiGALBindFlags::ShaderResource) ||
        !Buffer(context, emissiveTables[1], sizeof(xiiGpuEmissiveLightHeader), 1, xiiGALBindFlags::ShaderResource) ||
        !Buffer(context, emissiveTables[2], sizeof(xiiGpuEmissiveAliasEntry), capacity, xiiGALBindFlags::ShaderResource) ||
        !Buffer(context, emissiveTables[3], sizeof(xiiGpuEmissiveAliasEntry), (capacity + 255U) / 256U, xiiGALBindFlags::ShaderResource) ||
        !Buffer(context, emissiveTables[4], sizeof(xiiGpuEmissiveAliasHeader), 1, xiiGALBindFlags::ShaderResource))
      return context.ReportFailure("Ray irradiance requires matching native emissive header and alias tables.");
  }
  const auto environment         = m_uiEnvironmentLightSamples != 0 ? context.GetInputData(m_Environment).Downcast<const xiiReflectionProbeFrameData>() : xiiSharedPtr<const xiiReflectionProbeFrameData>();
  const bool environmentSampling = environment && environment->m_Sky;
  if (environmentSampling && !xiiScreenSpacePassUtils::Uniform(context, environment->m_SkyConstants, sizeof(xiiSkyLightConstants)))
    return context.ReportFailure("Ray irradiance requires the canonical sky constants paired with its environment source.");
  xiiSharedPtr<ReservoirState::Slot> nextReservoir, previousReservoir;
  xiiSharedPtr<xiiGALBuffer>         previousBuffer;
  xiiRenderGraphTextureHandle        receiverVelocity, receiverObject, receiverMaterial;
  xiiGIPathReservoirConstants        reservoirSettings{};
  xiiUInt64                          reservoirRevision = 0;
  if (m_bUsePathReservoirs && m_bAnimateSamples)
  {
    receiverVelocity              = context.GetInputTexture(m_ReceiverVelocity);
    receiverObject                = context.GetInputTexture(m_ReceiverObject);
    receiverMaterial              = context.GetInputTexture(m_ReceiverMaterial);
    const auto* objectDescription = context.m_Graph.GetTextureDescription(receiverObject);
    const auto  temporal          = context.GetInputData(m_TemporalCamera).Downcast<const xiiTemporalCameraSnapshot>();
    if (!temporal || temporal->m_Sample.m_Size != size || !xiiScreenSpacePassUtils::Image(context, receiverVelocity, 2, &size) ||
        !xiiScreenSpacePassUtils::Image(context, receiverMaterial, 4, &size) || !objectDescription || objectDescription->m_Size != size || objectDescription->m_Format != xiiGALResourceFormat::R32UInt)
      return context.ReportFailure("Path reservoirs require matching velocity, material, identity, and temporal camera inputs.");
    const xiiUInt64 frame     = context.m_Graph.GetFrameIndex();
    xiiUInt64       completed = 0;
    if (xiiRenderGraphManager::GetFrameCompletion(frame, completed).Failed())
      return context.ReportFailure("Path reservoirs require an active shared GPU frame.");

    xiiUInt32  tile   = 2;
    const auto divide = [](xiiUInt32 value, xiiUInt32 divisor) {
      return (value - 1) / divisor + 1;
    };
    while (xiiUInt64(divide(size.width, tile)) * divide(size.height, tile) * XII_GI_PATH_RESERVOIR_STRIDE > ReservoirState::s_uiBytesPerSlot)
      tile *= 2;
    const xiiUInt32 columns = divide(size.width, tile), rows = divide(size.height, tile);
    if (!m_ReservoirState)
      m_ReservoirState = XII_DEFAULT_NEW(ReservoirState);
    auto& state = *m_ReservoirState;
    XII_LOCK(state.m_Mutex);
    for (xiiUInt32 index = state.m_Slots.GetCount(); index-- > 0;)
    {
      const auto& slot = state.m_Slots[index];
      if (slot != state.m_Last && slot->GetRefCount() == 1 && slot->m_uiLastUsedFrame <= completed)
      {
        if (slot->m_Extent != size)
          state.m_Slots.RemoveAtAndSwap(index);
        else if (!nextReservoir)
          nextReservoir = slot;
      }
    }
    if (!nextReservoir && state.m_Slots.GetCount() < ReservoirState::s_uiMaxSlots)
    {
      xiiGALBufferCreationDescription storage;
      storage.m_Mode                = xiiGALBufferMode::Structured;
      storage.m_uiElementByteStride = XII_GI_PATH_RESERVOIR_STRIDE;
      storage.m_uiSize              = xiiUInt64(columns) * rows * XII_GI_PATH_RESERVOIR_STRIDE;
      storage.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      nextReservoir                 = XII_DEFAULT_NEW(ReservoirState::Slot);
      nextReservoir->m_Buffer       = device->CreateBuffer(storage);
      nextReservoir->m_Ready        = device->CreateFence({.m_Type = xiiGALFenceType::General});
      nextReservoir->m_Extent       = size;
      if (!nextReservoir->m_Buffer || !nextReservoir->m_Ready)
        return context.ReportFailure("Could not allocate path reservoir storage or readiness fence.");
      state.m_Slots.PushBack(nextReservoir);
    }
    // Retained recordings can consume every slot. This frame uses fresh paths
    // and leaves retained storage untouched until its GPU consumers finish.
    if (nextReservoir)
    {
      const auto& last = state.m_Last;
      if (last && last->m_Extent == size && temporal->m_Sample.m_bHistoryValid && last->m_uiFrame == temporal->m_Sample.m_uiPreviousFrame && last->m_uiFrame < frame)
      {
        previousReservoir       = last;
        last->m_uiLastUsedFrame = frame;
        previousBuffer          = last->m_Buffer;
      }
      else
      {
        if (!state.m_Empty)
        {
          xiiGALBufferCreationDescription storage;
          storage.m_Mode                = xiiGALBufferMode::Structured;
          storage.m_uiElementByteStride = XII_GI_PATH_RESERVOIR_STRIDE;
          storage.m_uiSize              = XII_GI_PATH_RESERVOIR_STRIDE;
          storage.m_BindFlags           = xiiGALBindFlags::ShaderResource;
          state.m_Empty                 = device->CreateBuffer(storage);
        }
        if (!state.m_Empty)
          return context.ReportFailure("Could not allocate the empty path reservoir binding.");
        previousBuffer = state.m_Empty;
      }
      nextReservoir->m_uiFrame = nextReservoir->m_uiLastUsedFrame = frame;
      ++nextReservoir->m_uiFenceValue;
      reservoirRevision                       = state.m_uiRevision;
      reservoirSettings.ReservoirGrid         = xiiVec4U32(columns, rows, tile, previousReservoir ? 1U : 0U);
      reservoirSettings.ReservoirViewport     = xiiVec4U32(size.width, size.height, 0, 0);
      const auto jitter                       = temporal->m_Sample.m_PreviousJitter - temporal->m_Sample.m_Jitter;
      reservoirSettings.ReservoirReprojection = xiiVec4(jitter.x, jitter.y, 0, 0);
    }
  }
  else
  {
    InvalidateTemporalHistory();
  }
  const bool      useReservoirs  = nextReservoir != nullptr;
  const xiiUInt32 recursionDepth = m_uiMaximumHits + (m_uiAnalyticLightSamples != 0 || environmentSampling ? 1U : 0U);
  if (!limits.m_uiShaderGroupHandleSize || !xiiMath::IsPowerOf2(limits.m_uiShaderGroupBaseAlignment) || !xiiMath::IsPowerOf2(limits.m_uiShaderGroupHandleAlignment) || limits.m_uiMaxRecursionDepth < recursionDepth)
    return context.ReportFailure("Ray irradiance exceeds shader record or recursion capabilities.");
  const auto stride = xiiMemoryUtils::AlignSize(xiiUInt64(limits.m_uiShaderGroupHandleSize), xiiUInt64(xiiMath::Max(limits.m_uiShaderGroupBaseAlignment, limits.m_uiShaderGroupHandleAlignment)));
  if (stride > limits.m_uiMaxShaderRecordStride || stride > xiiMath::MaxValue<xiiUInt32>() / 3U)
    return context.ReportFailure("Ray irradiance shader records exceed device limits.");
  constexpr xiiUInt32 freshStage = 0, seedStage = 1, resolveStage = 2;
  const auto          loadPipeline = [&](xiiUInt32 stage) -> xiiSharedPtr<xiiGALRayTracingPipelineState> {
    auto& shaderHandle = m_Shader[stage * 4U + (emissiveSampling ? 1U : 0U) + (environmentSampling ? 2U : 0U)];
    if (!shaderHandle.IsValid())
    {
      xiiHashTable<xiiHashedString, xiiHashedString> variables;
      xiiHashedString                                name, value;
      name.Assign("GI_EMISSIVE_SAMPLING");
      value.Assign(emissiveSampling ? "TRUE" : "FALSE");
      variables.Insert(name, value);
      name.Assign("GI_ENVIRONMENT_SAMPLING");
      value.Assign(environmentSampling ? "TRUE" : "FALSE");
      variables.Insert(name, value);
      name.Assign("GI_PATH_RESERVOIRS");
      const char* stages[] = {"GI_PATH_RESERVOIRS_NONE", "GI_PATH_RESERVOIRS_SEED", "GI_PATH_RESERVOIRS_RESOLVE"};
      value.Assign(stages[stage]);
      variables.Insert(name, value);
      shaderHandle = xiiShaderPermutationUtilities::PreloadSinglePermutation(xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/RayTracedIrradiance.xiiShader"), variables, false);
    }
    xiiResourceLock<xiiShaderPermutationResource> shader(shaderHandle, xiiResourceAcquireMode::BlockTillLoaded);
    if (!shader.IsValid() || !shader->IsShaderValid())
      return {};
    xiiGALRayTracingPipelineStateCreationDescription description;
    description.m_pPipelineResourceSignature               = shader->GetPipelineResourceSignature();
    description.m_RayTracingPipeline.m_uiMaxRecursionDepth = recursionDepth;
    description.m_uiMaximumPayloadSize                     = useReservoirs ? 64 : (emissiveSampling || environmentSampling ? 36 : 32);
    description.m_uiMaximumAttributeSize                   = 8;
    auto& generation                                       = description.m_GeneralShaders.ExpandAndGetRef();
    generation.m_sName.Assign("IrradianceGeneration");
    generation.m_pShader = shader->GetGALShader(xiiGALShaderType::RayGeneration);
    auto& miss           = description.m_GeneralShaders.ExpandAndGetRef();
    miss.m_sName.Assign("IrradianceMiss");
    miss.m_pShader = shader->GetGALShader(xiiGALShaderType::RayMiss);
    auto& hit      = description.m_TriangleHitShaders.ExpandAndGetRef();
    hit.m_sName.Assign("IrradianceHit");
    hit.m_pClosestHitShader = shader->GetGALShader(xiiGALShaderType::RayClosestHit);
    hit.m_pAnyHitShader     = shader->GetGALShader(xiiGALShaderType::RayAnyHit);
    return xiiGALPipelineCache::GetPipeline(description);
  };
  const auto pipeline     = loadPipeline(useReservoirs ? resolveStage : freshStage);
  const auto seedPipeline = useReservoirs ? loadPipeline(seedStage) : xiiSharedPtr<xiiGALRayTracingPipelineState>();
  if (!pipeline || (useReservoirs && !seedPipeline))
    return context.ReportFailure("Could not create ray irradiance seed or resolve pipeline.");
  if (!m_Sampler)
  {
    xiiGALSamplerCreationDescription sampler;
    sampler.m_MinFilter       = xiiGALFilterType::Anisotropic;
    sampler.m_MagFilter       = xiiGALFilterType::Anisotropic;
    sampler.m_MipFilter       = xiiGALFilterType::Linear;
    sampler.m_AddressU        = xiiGALTextureAddressMode::Wrap;
    sampler.m_AddressV        = xiiGALTextureAddressMode::Wrap;
    sampler.m_AddressW        = xiiGALTextureAddressMode::Wrap;
    sampler.m_uiMaxAnisotropy = 8;
    m_Sampler                 = device->CreateSampler(sampler);
    if (!m_Sampler)
      return context.ReportFailure("Could not allocate ray material fallback sampler.");
  }
  // Graph-local tables follow the current view's immutable candidate list.
  // Disabled sampling binds existing float storage but submits no scan work.
  xiiRenderGraphBufferHandle lightCDF = profiles, blockCDF = profiles, blockPower;
  const bool                 powerWeighted = m_bPowerWeightedLights && m_uiAnalyticLightSamples != 0 && selection->m_uiSize > sizeof(xiiUInt32);
  if (powerWeighted)
  {
    const xiiUInt32 candidateCount = xiiUInt32(selection->m_uiSize / sizeof(xiiUInt32));
    const xiiUInt32 blockCount     = (candidateCount + 255U) / 256U;
    const auto&     compute        = device->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
    if (compute.m_uiMaxThreadGroupInvocations < 256 || compute.m_uiMaxThreadGroupSizeX < 256 || compute.m_uiMaxThreadGroupCountX < blockCount)
      return context.ReportFailure("Ray light power scan exceeds compute group limits.");
    XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_LightScanPipeline, "Shaders/Pipeline/Passes/GlobalIllumination/LightPowerScan.xiiShader"));
    XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_BlockScanPipeline, "Shaders/Pipeline/Passes/GlobalIllumination/LightBlockScan.xiiShader"));
    struct ScanData
    {
      xiiRenderGraphBufferHandle               m_Lights, m_Counts, m_Candidates, m_CDF, m_Power;
      xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
      xiiUInt32                                m_Groups = 1;
      bool                                     m_Global = false;
    };
    for (xiiUInt32 stage = 0; stage < 2; ++stage)
    {
      const auto setup = [&](ScanData& data, xiiRenderGraphBuilder& builder) {
        data.m_Global   = stage != 0;
        data.m_Pipeline = data.m_Global ? m_BlockScanPipeline : m_LightScanPipeline;
        if (data.m_Global)
        {
          data.m_Power = builder.ReadBuffer(blockPower, xiiGALResourceStateFlags::ShaderResource);
          data.m_CDF   = builder.WriteBuffer("GI light block CDF", xiiLightClusterPassUtils::Description(sizeof(float), blockCount), xiiGALResourceStateFlags::UnorderedAccess);
        }
        else
        {
          data.m_Lights     = builder.ReadBuffer(lights, xiiGALResourceStateFlags::ShaderResource);
          data.m_Counts     = builder.ReadBuffer(counts, xiiGALResourceStateFlags::ShaderResource);
          data.m_Candidates = builder.ReadBuffer(candidates, xiiGALResourceStateFlags::ShaderResource);
          data.m_CDF        = builder.WriteBuffer("GI candidate power CDF", xiiLightClusterPassUtils::Description(sizeof(float), candidateCount), xiiGALResourceStateFlags::UnorderedAccess);
          data.m_Power      = builder.WriteBuffer("GI light block power", xiiLightClusterPassUtils::Description(sizeof(float), blockCount), xiiGALResourceStateFlags::UnorderedAccess);
          data.m_Groups     = blockCount;
        }
      };
      const auto record = [](const ScanData& data, xiiRenderGraphPassContext& execute) {
        auto& commands = execute.GetCommandList();
        commands.SetPipelineState(data.m_Pipeline.Borrow());
        const auto bind = [&](xiiStringView name, xiiRenderGraphBufferHandle buffer) {
          commands.ResolveAndSetShaderResourceBufferView(xiiTempHashedString(name), execute.GetBuffer(buffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
        };
        if (data.m_Global)
        {
          bind("g_BlockPower", data.m_Power);
          commands.ResolveAndSetUnorderedAccessBufferView("g_BlockCDF", execute.GetBuffer(data.m_CDF)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
        }
        else
        {
          bind("g_Lights", data.m_Lights);
          bind("g_LightCounts", data.m_Counts);
          bind("g_LightCandidates", data.m_Candidates);
          commands.ResolveAndSetUnorderedAccessBufferView("g_LightCDF", execute.GetBuffer(data.m_CDF)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
          commands.ResolveAndSetUnorderedAccessBufferView("g_BlockPower", execute.GetBuffer(data.m_Power)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
        }
        if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
        {
          execute.ReportFailure("Could not bind ray light power scan resources.");
          return;
        }
        xiiGALDispatchComputeDescription dispatch;
        dispatch.m_uiThreadGroupCountX = data.m_Groups;
        commands.DispatchCompute(dispatch);
      };
      const auto pass = context.m_Graph.AddPass<ScanData>(stage == 0 ? "GI candidate power scan" : "GI block power scan", m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, setup, record);
      if (stage == 0)
      {
        lightCDF   = pass.first->m_CDF;
        blockPower = pass.first->m_Power;
      }
      else
        blockCDF = pass.first->m_CDF;
    }
  }
  xiiRenderGraphTextureHandle environmentSource;
  xiiRenderGraphBufferHandle  environmentCDF, environmentRowCDF;
  if (environmentSampling)
  {
    const auto& compute = device->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
    if (compute.m_uiMaxThreadGroupInvocations < 64 || compute.m_uiMaxThreadGroupSizeX < 64 || compute.m_uiMaxThreadGroupCountX < 32)
      return context.ReportFailure("Environment power scan exceeds compute group limits.");
    XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LinearClampSampler(m_EnvironmentSampler));
    XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_EnvironmentScanPipeline, "Shaders/Pipeline/Passes/GlobalIllumination/EnvironmentPowerScan.xiiShader"));
    XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_BlockScanPipeline, "Shaders/Pipeline/Passes/GlobalIllumination/LightBlockScan.xiiShader"));
    struct EnvironmentScanData
    {
      xiiRenderGraphTextureHandle              m_Source;
      xiiRenderGraphBufferHandle               m_CDF, m_Power;
      xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
      xiiSharedPtr<xiiGALSampler>              m_Sampler;
      bool                                     m_bRows;
    };
    xiiRenderGraphBufferHandle rowPower;
    for (xiiUInt32 stage = 0; stage < 2; ++stage)
    {
      const auto setupEnvironment = [&](EnvironmentScanData& data, xiiRenderGraphBuilder& builder) {
        data.m_bRows    = stage != 0;
        data.m_Pipeline = data.m_bRows ? m_BlockScanPipeline : m_EnvironmentScanPipeline;
        data.m_Sampler  = m_EnvironmentSampler;
        if (data.m_bRows)
        {
          data.m_Power = builder.ReadBuffer(rowPower, xiiGALResourceStateFlags::ShaderResource);
          data.m_CDF   = builder.WriteBuffer("GI environment row CDF", xiiLightClusterPassUtils::Description(sizeof(float), 32), xiiGALResourceStateFlags::UnorderedAccess);
        }
        else
        {
          builder.WaitForFence(environment->m_Sky->m_Ready, 1).AssertSuccess();
          data.m_Source = builder.ReadTexture(builder.ImportTexture("GI sky source", environment->m_Sky->m_Source, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource);
          data.m_CDF    = builder.WriteBuffer("GI environment cell CDF", xiiLightClusterPassUtils::Description(sizeof(float), 2048), xiiGALResourceStateFlags::UnorderedAccess);
          data.m_Power  = builder.WriteBuffer("GI environment row power", xiiLightClusterPassUtils::Description(sizeof(float), 32), xiiGALResourceStateFlags::UnorderedAccess);
        }
      };
      const auto recordEnvironment = [](const EnvironmentScanData& data, xiiRenderGraphPassContext& execute) {
        auto& commands = execute.GetCommandList();
        commands.SetPipelineState(data.m_Pipeline.Borrow());
        if (data.m_bRows)
        {
          commands.ResolveAndSetShaderResourceBufferView("g_BlockPower", execute.GetBuffer(data.m_Power)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
          commands.ResolveAndSetUnorderedAccessBufferView("g_BlockCDF", execute.GetBuffer(data.m_CDF)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
        }
        else
        {
          commands.ResolveAndSetShaderResourceTextureView("g_GIEnvironment", execute.GetTexture(data.m_Source)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
          commands.ResolveAndSetSampler("g_sGIEnvironmentLinear", data.m_Sampler.Borrow());
          commands.ResolveAndSetUnorderedAccessBufferView("g_EnvironmentCDF", execute.GetBuffer(data.m_CDF)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
          commands.ResolveAndSetUnorderedAccessBufferView("g_BlockPower", execute.GetBuffer(data.m_Power)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
        }
        if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
        {
          execute.ReportFailure("Could not bind environment proposal scan resources.");
          return;
        }
        commands.DispatchCompute({data.m_bRows ? 1U : 32U, 1, 1});
      };
      const auto scan = context.m_Graph.AddPass<EnvironmentScanData>(stage == 0 ? "GI sky cell power scan" : "GI sky row power scan", m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, setupEnvironment, recordEnvironment);
      if (stage == 0)
      {
        environmentSource = scan.first->m_Source;
        environmentCDF    = scan.first->m_CDF;
        rowPower          = scan.first->m_Power;
      }
      else
        environmentRowCDF = scan.first->m_CDF;
    }
  }
  struct Data
  {
    xiiRenderGraphTextureHandle                 m_Depth, m_Normal, m_Output;
    xiiRenderGraphBufferHandle                  m_Dependency, m_Camera, m_Constants, m_Materials, m_Geometry, m_Records;
    xiiRenderGraphBufferHandle                  m_Lights, m_LightCounts, m_LightCandidates, m_IESProfiles;
    xiiRenderGraphBufferHandle                  m_LightCDF, m_BlockCDF;
    xiiRenderGraphBufferHandle                  m_EmissiveTables[5];
    xiiRenderGraphTextureHandle                 m_EnvironmentSource;
    xiiRenderGraphBufferHandle                  m_EnvironmentCDF, m_EnvironmentRowCDF, m_SkyConstants;
    xiiSharedPtr<xiiGALSampler>                 m_EnvironmentSampler;
    xiiRenderGraphTextureHandle                 m_ReceiverVelocity, m_ReceiverObject, m_ReceiverMaterial;
    xiiRenderGraphBufferHandle                  m_PreviousReservoirs, m_SpatialReservoirs, m_CurrentReservoirs, m_ReservoirConstants;
    bool                                        m_bSeed = false;
    xiiSharedPtr<ReservoirState>                m_ReservoirState;
    xiiSharedPtr<ReservoirState::Slot>          m_PreviousReservoirSlot, m_CurrentReservoirSlot;
    xiiGIPathReservoirConstants                 m_ReservoirSettings;
    xiiUInt64                                   m_uiReservoirRevision = 0;
    xiiRayTracedIrradianceConstants             m_Settings;
    xiiSharedPtr<xiiGALTopLevelAS>              m_Scene;
    xiiSharedPtr<xiiGALRayTracingPipelineState> m_Pipeline;
    xiiSharedPtr<xiiGALSampler>                 m_Sampler;
    xiiSizeU32                                  m_Size;
    xiiUInt64                                   m_Stride;
  };
  xiiRenderGraphBufferHandle currentReservoirs;
  const auto                 setup = [&](Data& data, xiiRenderGraphBuilder& builder, bool seed) {
    data.m_bSeed           = seed;
    data.m_Depth           = builder.ReadTexture(depth, xiiGALResourceStateFlags::ShaderResource);
    data.m_Normal          = builder.ReadTexture(normal, xiiGALResourceStateFlags::ShaderResource);
    data.m_Camera          = builder.ReadBuffer(camera, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Dependency      = builder.ReadBuffer(scene.m_Dependency, xiiGALResourceStateFlags::BuildASRead);
    data.m_Materials       = builder.ReadBuffer(materials, xiiGALResourceStateFlags::ShaderResource);
    data.m_Geometry        = builder.ReadBuffer(geometry, xiiGALResourceStateFlags::ShaderResource);
    data.m_Lights          = builder.ReadBuffer(lights, xiiGALResourceStateFlags::ShaderResource);
    data.m_LightCounts     = builder.ReadBuffer(counts, xiiGALResourceStateFlags::ShaderResource);
    data.m_LightCandidates = builder.ReadBuffer(candidates, xiiGALResourceStateFlags::ShaderResource);
    data.m_IESProfiles     = builder.ReadBuffer(profiles, xiiGALResourceStateFlags::ShaderResource);
    data.m_LightCDF        = builder.ReadBuffer(lightCDF, xiiGALResourceStateFlags::ShaderResource);
    data.m_BlockCDF        = builder.ReadBuffer(blockCDF, xiiGALResourceStateFlags::ShaderResource);
    if (emissiveSampling)
      for (xiiUInt32 index = 0; index < 5; ++index)
        data.m_EmissiveTables[index] = builder.ReadBuffer(emissiveTables[index], xiiGALResourceStateFlags::ShaderResource);
    data.m_Scene = scene.m_pStructure;
    if (!seed)
      data.m_Output = xiiCompositionPassUtils::Output(builder, size, xiiGALResourceFormat::RGBA32Float);
    data.m_Constants = xiiRenderPassUtils::Constants(builder, "Ray irradiance settings", sizeof(data.m_Settings));
    if (environmentSampling)
    {
      data.m_EnvironmentSource  = builder.ReadTexture(environmentSource, xiiGALResourceStateFlags::ShaderResource);
      data.m_EnvironmentCDF     = builder.ReadBuffer(environmentCDF, xiiGALResourceStateFlags::ShaderResource);
      data.m_EnvironmentRowCDF  = builder.ReadBuffer(environmentRowCDF, xiiGALResourceStateFlags::ShaderResource);
      data.m_SkyConstants       = builder.ReadBuffer(environment->m_SkyConstants, xiiGALResourceStateFlags::ConstantBuffer);
      data.m_EnvironmentSampler = m_EnvironmentSampler;
    }
    if (useReservoirs)
    {
      data.m_ReservoirState       = m_ReservoirState;
      data.m_CurrentReservoirSlot = nextReservoir;
      data.m_uiReservoirRevision  = reservoirRevision;
      data.m_ReservoirSettings    = reservoirSettings;
      data.m_ReservoirConstants   = xiiRenderPassUtils::Constants(builder, "GI path reservoir settings", sizeof(xiiGIPathReservoirConstants));
      data.m_ReceiverObject       = builder.ReadTexture(receiverObject, xiiGALResourceStateFlags::ShaderResource);
      data.m_ReceiverMaterial     = builder.ReadTexture(receiverMaterial, xiiGALResourceStateFlags::ShaderResource);
      if (seed)
      {
        data.m_CurrentReservoirs = builder.WriteBuffer(builder.ImportBuffer("Current GI paths", nextReservoir->m_Buffer, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::UnorderedAccess);
      }
      else
      {
        if (previousReservoir)
          builder.WaitForFence(previousReservoir->m_Ready, previousReservoir->m_uiFenceValue).AssertSuccess();
        data.m_PreviousReservoirSlot = previousReservoir;
        data.m_PreviousReservoirs    = builder.ReadBuffer(builder.ImportBuffer("Previous GI paths", previousBuffer, xiiGALResourceStateFlags::Unknown), xiiGALResourceStateFlags::ShaderResource);
        data.m_SpatialReservoirs     = builder.ReadBuffer(currentReservoirs, xiiGALResourceStateFlags::ShaderResource);
        data.m_ReceiverVelocity      = builder.ReadTexture(receiverVelocity, xiiGALResourceStateFlags::ShaderResource);
      }
    }
    data.m_Settings.RayDistances    = xiiVec4(m_fRayLength, m_fSurfaceBias, powerWeighted ? 1.0f : 0.0f, 0);
    data.m_Settings.EmissiveSamples = xiiVec4U32(m_uiEmissiveLightSamples, xiiUInt32(context.m_Graph.GetFrameIndex()), environmentSampling ? m_uiEnvironmentLightSamples : 0U, 0);
    data.m_Settings.PathSamples     = xiiVec4U32(m_uiSampleCount, m_uiMaximumHits, m_uiSampleOffset + (m_bAnimateSamples ? xiiUInt32(context.m_Graph.GetFrameIndex()) * m_uiSampleCount : 0U), m_uiAnalyticLightSamples);
    data.m_Size                     = seed ? xiiSizeU32(reservoirSettings.ReservoirGrid.x, reservoirSettings.ReservoirGrid.y) : size;
    data.m_Stride                   = stride;
    data.m_Pipeline                 = seed ? seedPipeline : pipeline;
    data.m_Sampler                  = m_Sampler;
    xiiGALBufferCreationDescription records;
    records.m_uiSize    = 3 * stride;
    records.m_Mode      = xiiGALBufferMode::Raw;
    records.m_Usage     = xiiGALResourceUsage::Mutable;
    records.m_BindFlags = xiiGALBindFlags::RayTracing;
    data.m_Records      = builder.WriteBuffer("Irradiance shader records", records, xiiGALResourceStateFlags::RayTracing);
  };
  const auto record = [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    {
      xiiGALMapHelper<xiiRayTracedIrradianceConstants> mapped(commands, execute.GetBuffer(data.m_Constants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *mapped = data.m_Settings;
    }
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetAccelerationStructure("g_Scene", data.m_Scene.Borrow(), xiiGALShaderType::RayGeneration | xiiGALShaderType::RayClosestHit);
    commands.ResolveAndSetConstantBuffer("xiiSurfaceCameraConstants", execute.GetBuffer(data.m_Camera), xiiGALShaderType::RayGeneration);
    commands.ResolveAndSetConstantBuffer("xiiRayTracedIrradianceConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::RayGeneration | xiiGALShaderType::RayClosestHit);
    commands.ResolveAndSetShaderResourceTextureView("g_Depth", execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
    commands.ResolveAndSetShaderResourceTextureView("g_Normal", execute.GetTexture(data.m_Normal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
    if (!data.m_bSeed)
      commands.ResolveAndSetUnorderedAccessTextureView("g_Irradiance", execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
    const auto stages = xiiGALShaderType::RayClosestHit | xiiGALShaderType::RayAnyHit;
    commands.ResolveAndSetShaderResourceBufferView("g_RayTracingMaterials", execute.GetBuffer(data.m_Materials)->GetDefaultView(xiiGALBufferViewType::ShaderResource), stages);
    commands.ResolveAndSetShaderResourceBufferView("g_RayTracingGeometry", execute.GetBuffer(data.m_Geometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), stages);
    commands.ResolveAndSetShaderResourceBufferView("g_Lights", execute.GetBuffer(data.m_Lights)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
    commands.ResolveAndSetShaderResourceBufferView("g_LightCounts", execute.GetBuffer(data.m_LightCounts)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
    commands.ResolveAndSetShaderResourceBufferView("g_LightCandidates", execute.GetBuffer(data.m_LightCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
    commands.ResolveAndSetShaderResourceBufferView("g_IESProfiles", execute.GetBuffer(data.m_IESProfiles)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
    commands.ResolveAndSetShaderResourceBufferView("g_LightCDF", execute.GetBuffer(data.m_LightCDF)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
    commands.ResolveAndSetShaderResourceBufferView("g_BlockCDF", execute.GetBuffer(data.m_BlockCDF)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
    if (data.m_Settings.EmissiveSamples.x != 0)
    {
      const xiiTempHashedString names[]       = {"g_EmissiveTriangles", "g_EmissiveHeader", "g_EmissiveTriangleAliases", "g_EmissiveBlockAliases", "g_EmissiveAliasHeader"};
      const auto                emitterStages = data.m_bSeed ? xiiBitflags<xiiGALShaderType>(xiiGALShaderType::RayClosestHit) : xiiGALShaderType::RayGeneration | xiiGALShaderType::RayClosestHit;
      for (xiiUInt32 index = 0; index < 5; ++index)
        commands.ResolveAndSetShaderResourceBufferView(names[index], execute.GetBuffer(data.m_EmissiveTables[index])->GetDefaultView(xiiGALBufferViewType::ShaderResource), emitterStages);
    }
    if (data.m_SkyConstants.IsValid())
    {
      const auto environmentStages = xiiGALShaderType::RayClosestHit | xiiGALShaderType::RayMiss;
      commands.ResolveAndSetConstantBuffer("xiiRayTracedIrradianceConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::RayMiss);
      commands.ResolveAndSetConstantBuffer("xiiSkyLightConstants", execute.GetBuffer(data.m_SkyConstants), environmentStages);
      commands.ResolveAndSetShaderResourceTextureView("g_GIEnvironment", execute.GetTexture(data.m_EnvironmentSource)->GetDefaultView(xiiGALTextureViewType::ShaderResource), environmentStages);
      commands.ResolveAndSetShaderResourceBufferView("g_EnvironmentCDF", execute.GetBuffer(data.m_EnvironmentCDF)->GetDefaultView(xiiGALBufferViewType::ShaderResource), environmentStages);
      commands.ResolveAndSetShaderResourceBufferView("g_EnvironmentRowCDF", execute.GetBuffer(data.m_EnvironmentRowCDF)->GetDefaultView(xiiGALBufferViewType::ShaderResource), environmentStages);
      commands.ResolveAndSetSampler("g_sGIEnvironmentLinear", data.m_EnvironmentSampler.Borrow());
    }
    if (data.m_CurrentReservoirSlot)
    {
      {
        xiiGALMapHelper<xiiGIPathReservoirConstants> mapped(commands, execute.GetBuffer(data.m_ReservoirConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *mapped = data.m_ReservoirSettings;
      }
      constexpr auto generation = xiiGALShaderType::RayGeneration;
      commands.ResolveAndSetConstantBuffer("xiiGIPathReservoirConstants", execute.GetBuffer(data.m_ReservoirConstants), generation);
      if (data.m_bSeed)
        commands.ResolveAndSetUnorderedAccessBufferView("g_CurrentReservoirs", execute.GetBuffer(data.m_CurrentReservoirs)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), generation);
      else
      {
        commands.ResolveAndSetShaderResourceBufferView("g_PreviousReservoirs", execute.GetBuffer(data.m_PreviousReservoirs)->GetDefaultView(xiiGALBufferViewType::ShaderResource), generation);
        commands.ResolveAndSetShaderResourceBufferView("g_SpatialReservoirs", execute.GetBuffer(data.m_SpatialReservoirs)->GetDefaultView(xiiGALBufferViewType::ShaderResource), generation);
        commands.ResolveAndSetShaderResourceTextureView("g_ReceiverVelocity", execute.GetTexture(data.m_ReceiverVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), generation);
      }
      commands.ResolveAndSetShaderResourceTextureView("g_ReceiverObject", execute.GetTexture(data.m_ReceiverObject)->GetDefaultView(xiiGALTextureViewType::ShaderResource), generation);
      commands.ResolveAndSetShaderResourceTextureView("g_ReceiverMaterial", execute.GetTexture(data.m_ReceiverMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), generation);
    }
    commands.ResolveAndSetSampler("g_sAnisotropicWrap", data.m_Sampler.Borrow());
    xiiGALBindlessResourceTable::BindBufferSRVs(commands, "g_RayTracingBuffers", stages);
    xiiGALBindlessResourceTable::BindTextureSRVs(commands, "g_RayTracingTextures", stages);
    xiiGALBindlessResourceTable::BindSamplers(commands, "g_MaterialSamplers", stages);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
    {
      execute.ReportFailure("Could not bind ray irradiance resources.");
      return;
    }
    xiiGALUpdateSBTDescription records;
    records.m_pPipelineState      = data.m_Pipeline.Borrow();
    records.m_pShaderBindingTable = execute.GetBuffer(data.m_Records);
    records.m_RayGenerationTable  = {0, data.m_Stride, data.m_Stride};
    records.m_MissTable           = {data.m_Stride, data.m_Stride, data.m_Stride};
    records.m_HitTable            = {2 * data.m_Stride, data.m_Stride, data.m_Stride};
    commands.UpdateSBT(records);
    xiiGALTraceRaysDescription trace(records.m_pShaderBindingTable, data.m_Size.width, data.m_Size.height);
    trace.m_RayGenerationTable = records.m_RayGenerationTable;
    trace.m_MissTable          = records.m_MissTable;
    trace.m_HitTable           = records.m_HitTable;
    commands.TraceRays(trace);
    if (data.m_bSeed)
    {
      const auto& slot = data.m_CurrentReservoirSlot;
      commands.EnqueueSignal(slot->m_Ready.Borrow(), slot->m_uiFenceValue);
      auto& state = *data.m_ReservoirState;
      XII_LOCK(state.m_Mutex);
      if (data.m_uiReservoirRevision == state.m_uiRevision && (!state.m_Last || slot->m_uiFrame > state.m_Last->m_uiFrame))
        state.m_Last = slot;
    }
  };
  const auto queue = m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics;
  if (useReservoirs)
  {
    const auto seedSetup = [&](Data& data, xiiRenderGraphBuilder& builder) {
      setup(data, builder, true);
    };
    const auto seed   = context.m_Graph.AddPass<Data>("Trace Current Diffuse Path Reservoirs", queue, seedSetup, record);
    currentReservoirs = seed.first->m_CurrentReservoirs;
  }
  const auto resolveSetup = [&](Data& data, xiiRenderGraphBuilder& builder) {
    setup(data, builder, false);
  };
  const auto passName = useReservoirs ? (previousReservoir ? "Resolve Current Spatial and Temporal GI" : "Resolve Current Spatial GI") : context.m_sPassName;
  const auto pass     = context.m_Graph.AddPass<Data>(passName, queue, resolveSetup, record);
  return context.SetOutput(m_Irradiance, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_Implementation_RayTracedIrradiancePass);
