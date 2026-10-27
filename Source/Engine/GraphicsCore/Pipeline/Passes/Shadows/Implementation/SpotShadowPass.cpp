/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <GraphicsCore/Lighting/LocalShadow.h>
#include <GraphicsCore/Lighting/WorldLightData.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/SpotShadowPass.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Shadows/SpotShadowConstants.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSpotShadowPass, 1, xiiRTTIDefaultAllocator<xiiSpotShadowPass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("SpotShadows", m_Shadows)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_MEMBER_PROPERTY("Resolution", m_uiResolution)->AddAttributes(new xiiDefaultValueAttribute(1024U)),
      XII_MEMBER_PROPERTY("MaximumLights", m_uiMaximumLights)->AddAttributes(new xiiDefaultValueAttribute(4U)),
      XII_MEMBER_PROPERTY("NearPlane", m_fNearPlane)->AddAttributes(new xiiDefaultValueAttribute(0.02f)),
      XII_MEMBER_PROPERTY("DepthBias", m_fDepthBias)->AddAttributes(new xiiDefaultValueAttribute(0.0005f)),
      XII_MEMBER_PROPERTY("NormalBias", m_fNormalBias)->AddAttributes(new xiiDefaultValueAttribute(0.01f)),
      XII_MEMBER_PROPERTY("NormalBiasTexels", m_fNormalBiasTexels)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    } XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {new xiiCategoryAttribute("Rendering/Shadows")} XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiResult xiiSpotShadowPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiResolution >= 64 && m_uiResolution <= 16384 && m_uiMaximumLights > 0 && m_uiMaximumLights <= 4 &&
      xiiMath::IsFinite(m_fNearPlane) && m_fNearPlane > 0 && xiiMath::IsFinite(m_fDepthBias) && m_fDepthBias >= 0 && m_fDepthBias <= 0.1f &&
      xiiMath::IsFinite(m_fNormalBias) && m_fNormalBias >= 0 && m_fNormalBias <= 10 && xiiMath::IsFinite(m_fNormalBiasTexels) && m_fNormalBiasTexels >= 0 && m_fNormalBiasTexels <= 4 ?
    XII_SUCCESS :
    XII_FAILURE;
}

xiiResult xiiSpotShadowPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!m_bEnabled)
    return XII_SUCCESS;
  if (!context.m_pView || !context.m_pView->GetWorld() || !context.m_pView->GetExtractedRenderData() || !context.m_pView->GetGpuInstanceData())
    return context.ReportFailure("Spotlight shadows require an extracted authoritative view.");
  const auto* world  = static_cast<const xiiWorld*>(context.m_pView->GetWorld())->GetModule<xiiRenderWorldModule>();
  const auto* lights = world ? world->GetWorldLightData() : nullptr;
  if (!lights)
    return context.ReportFailure("Spotlight shadows require canonical world light extraction.");
  xiiHashSet<xiiUInt32>      seen;
  xiiDynamicArray<xiiUInt32> selected;
  const auto                 records = lights->GetLights();
  for (const auto* packet : context.m_pView->GetExtractedRenderData()->GetAllRenderData())
  {
    const auto index = lights->GetIndex(packet->m_hOwnerComponent);
    if (index >= records.GetCount() || seen.Contains(index))
      continue;
    const auto& light = records[index];
    if (light.m_Metadata.z != xiiUInt32(xiiGpuLightType::Spot) || light.m_ShadowData.x <= 0 || light.m_ColorAndIntensity.w <= 0)
      continue;
    const float fadeDistance = light.m_ShadowData.y > 0 ? light.m_ShadowData.y : light.m_AttenuationAndSize.x;
    if (light.m_ShadowData.y >= 0 && xiiMath::IsFinite(fadeDistance) && fadeDistance > 0 && context.m_pView->GetCamera() && (light.m_PositionAndInvRange.GetAsVec3() - context.m_pView->GetCamera()->GetPosition()).GetLength() >= fadeDistance)
      continue;
    seen.Insert(index);
    selected.PushBack(index);
  }
  if (selected.IsEmpty())
    return XII_SUCCESS;
  if (selected.GetCount() > m_uiMaximumLights)
    return context.ReportFailure("Spotlight shadow capacity exceeded; increase MaximumLights up to four or disable this workload explicitly.");
  if (!context.m_pView->GetCamera() || context.m_pView->GetCamera()->IsStereoscopic())
    return context.ReportFailure("Spotlight shadows require an explicit non-stereo camera view.");
  selected.Sort();
  xiiStaticArray<xiiSpotShadowDescription, 4> views;
  xiiStaticArray<xiiGpuLightData, 4>          projectedLights;
  for (const auto index : selected)
  {
    const auto& light     = records[index];
    const auto  position  = light.m_PositionAndInvRange.GetAsVec3();
    const auto  direction = light.m_DirectionAndType.GetAsVec3();
    const auto  right     = light.m_OrientationRightAndIES.GetAsVec3();
    const float range = light.m_AttenuationAndSize.x, cosine = light.m_SpotAnglesAndRectSize.y;
    if (!position.IsValid() || !direction.IsValid() || !right.IsValid() || !direction.IsNormalized(0.01f) || !right.IsNormalized(0.01f) || !xiiMath::IsFinite(range) || range <= m_fNearPlane * 2 || !xiiMath::IsFinite(cosine) || cosine < xiiMath::Cos(xiiAngle::MakeFromDegree(87.5f)) || cosine > xiiMath::Cos(xiiAngle::MakeFromDegree(0.5f)) || !xiiMath::IsFinite(light.m_ShadowData.y) || light.m_ShadowData.y < 0)
      return context.ReportFailure("Spotlight shadows require a finite 1-to-175-degree cone and a range beyond twice NearPlane.");
    xiiSpotShadowDescription view;
    view.m_Position      = position;
    view.m_uiLightIndex  = index;
    view.m_fFadeDistance = light.m_ShadowData.y > 0 ? light.m_ShadowData.y : range;
    view.m_fTanHalfAngle = xiiMath::Sqrt(1 - cosine * cosine) / cosine;
    auto up              = direction.CrossRH(right);
    if (up.NormalizeIfNotZero().Failed())
      return context.ReportFailure("Spotlight shadow orientation requires independent forward and right axes.");
    projectedLights.PushBack(light);
    views.PushBack(view);
  }
  // Reuse the existing local-shadow projection planner. The authored caster
  // occurrences own slot placement; only its per-light projections are needed.
  xiiLocalShadowAtlasSettings settings;
  settings.m_uiAtlasSize        = m_uiResolution * 2;
  settings.m_uiTileSize         = m_uiResolution;
  settings.m_uiMaxFacesPerFrame = views.GetCount();
  settings.m_fMinimumNearPlane  = m_fNearPlane;
  xiiLocalShadowAtlasDataArray  projections;
  xiiLocalShadowAtlasStatistics statistics;
  xiiLocalShadowAtlasBuilder::Build(settings, projectedLights, context.m_pView->GetCamera()->GetPosition(), projections, statistics);
  if (statistics.m_uiAllocatedLights != views.GetCount() || statistics.m_uiDroppedLights != 0)
    return context.ReportFailure("Spotlight projection planning did not cover every selected light.");
  for (xiiUInt32 i = 0; i < views.GetCount(); ++i)
  {
    views[i].m_ViewProjection = projections[i].m_ViewProjection[0];
    if (!views[i].m_ViewProjection.IsValid())
      return context.ReportFailure("Spotlight projection planning produced nonfinite values.");
  }
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiSpotShadowConstants      m_Constants{};
    xiiDynamicArray<xiiVec2U32> m_Flags;
    xiiRenderGraphBufferHandle  m_Uniform, m_Receivers;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Constants.SpotShadowLayout = xiiVec4U32(views.GetCount(), m_uiResolution, views.GetCount() > 1 ? 2U : 1U, views.GetCount() > 2 ? 2U : 1U);
    data.m_Constants.SpotShadowBias = xiiVec4(m_fDepthBias, m_fNormalBias, m_fNormalBiasTexels, 0);
    data.m_Constants.SpotLightIndices = xiiVec4U32(xiiInvalidIndex);
    for (auto& matrix : data.m_Constants.SpotViewProjection) matrix = xiiMat4::MakeIdentity();
    for (xiiUInt32 i = 0; i < views.GetCount(); ++i)
    {
      data.m_Constants.SpotViewProjection[i] = views[i].m_ViewProjection;
      data.m_Constants.SpotPositionAndTexelScale[i] = views[i].m_Position.GetAsVec4(2 * views[i].m_fTanHalfAngle / m_uiResolution);
      data.m_Constants.SpotFadeDistances.GetData()[i] = views[i].m_fFadeDistance;
      data.m_Constants.SpotLightIndices.GetData()[i] = views[i].m_uiLightIndex;
    }
    for (const auto& instance : context.m_pView->GetGpuInstanceData()->GetInstances())
      data.m_Flags.PushBack({instance.m_uiObjectId, instance.m_uiFlags});
    data.m_Flags.Sort([](const xiiVec2U32& a, const xiiVec2U32& b) { return a.x < b.x; });
    if (data.m_Flags.IsEmpty()) data.m_Flags.PushBack({0, 0});
    data.m_Uniform = xiiRenderPassUtils::Constants(builder, "Spotlight Sampling", sizeof(xiiSpotShadowConstants));
    xiiGALBufferCreationDescription receiver;
    receiver.m_Mode = xiiGALBufferMode::Structured;
    receiver.m_BindFlags = xiiGALBindFlags::ShaderResource;
    receiver.m_uiElementByteStride = sizeof(xiiVec2U32);
    receiver.m_uiSize = data.m_Flags.GetCount() * sizeof(xiiVec2U32);
    data.m_Receivers = builder.WriteBuffer("Spotlight Receivers", receiver, xiiGALResourceStateFlags::CopyDestination); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    xiiGALMapHelper<xiiSpotShadowConstants> mapped(commands, execute.GetBuffer(data.m_Uniform), xiiGALMapType::Write, xiiGALMapFlags::Discard);
    *mapped = data.m_Constants;
    commands.UpdateBuffer(execute.GetBuffer(data.m_Receivers), 0, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Flags.GetData()), data.m_Flags.GetCount() * sizeof(xiiVec2U32))); });
  return context.SetOutput(m_Shadows, XII_DEFAULT_NEW(xiiSpotShadowSnapshot, context.m_Graph.GetSetupSerial(), views, m_uiResolution, emitted.first->m_Uniform, emitted.first->m_Receivers));
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Shadows_Implementation_SpotShadowPass);
