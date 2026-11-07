/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/World.h>
#include <GraphicsCore/Lighting/EmissiveLightManager.h>
#include <GraphicsCore/Lighting/RayTracingResources.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/RayTracing/RayTracingScenePass.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>

struct xiiRayTracingScenePass::State
{
  xiiRayTracingFrameResources m_Frames;
};
xiiRayTracingScenePass::xiiRayTracingScenePass()  = default;
xiiRayTracingScenePass::~xiiRayTracingScenePass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRayTracingScenePass, 1, xiiRTTIDefaultAllocator<xiiRayTracingScenePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("BuildEmissiveLights", m_bBuildEmissiveLights)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("EmissiveTriangles", m_EmissiveTriangles)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("EmissiveHeader", m_EmissiveHeader)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("TriangleAliases", m_TriangleAliases)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("BlockAliases", m_BlockAliases)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AliasHeader", m_AliasHeader)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Activation", m_Activation)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("RequireActivation", m_bRequireActivation)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("Scene", m_OutputScene)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Materials", m_OutputMaterials)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Geometry", m_OutputGeometry)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("UseViewSelection", m_bUseViewSelection)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("RequireHardwareRayTracing", m_bRequireHardwareRayTracing)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("OnlyDiagnosticViews", m_bOnlyDiagnosticViews)->AddAttributes(new xiiDefaultValueAttribute(false)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/RayTracing") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiRayTracingScenePass::Build(xiiRenderPipelinePassContext& context)
{
  m_Statistics = {};
  if (m_bRequireActivation && !context.GetInputTexture(m_Activation).IsValid())
    return XII_SUCCESS;
  if (m_bOnlyDiagnosticViews && (!context.m_pView || !xiiViewRenderMode::IsRayTracingVisualizationMode(context.m_pView->GetViewRenderMode())))
    return XII_SUCCESS;
  if (!context.m_pView || !context.m_pView->GetWorld())
    return XII_FAILURE;
  if (!xiiRayTracingResources::IsHardwareRayTracingSupported())
    return m_bRequireHardwareRayTracing ? XII_FAILURE : XII_SUCCESS;
  const auto* module = static_cast<const xiiWorld*>(context.m_pView->GetWorld())->GetModule<xiiRenderWorldModule>();
  if (!module || !module->GetGpuInstanceData())
    return XII_FAILURE;
  if (module->GetGpuInstanceData()->GetInstanceCount() == 0)
    return XII_SUCCESS;
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  xiiRenderPassResourceScope scope(context);
  const auto                 selection = context.m_pView->GetGpuInstanceSelection();
  const auto                 built     = module->AddRayTracingBuildPass(context.m_Graph, context.m_Graph.GetFrameIndex(), &m_pState->m_Frames, m_bUseViewSelection ? &selection : nullptr);
  m_Statistics                         = {built.m_uiInstanceCount, built.m_uiDeformationDispatchCount, built.m_uiDynamicBLASBuildCount};
  if (built.m_Result.Failed())
    return context.ReportFailure("World-derived ray-tracing resources could not be built.");
  if (!built.m_pTopLevelAS)
    return XII_SUCCESS;
  if (m_bBuildEmissiveLights)
  {
    xiiEmissiveLightManager::BuildDescription description;
    description.m_hRayGeometry                = built.m_hGeometryData;
    description.m_hRayMaterials               = built.m_hMaterialData;
    description.m_hRaySceneDependency         = built.m_hSceneDependency;
    description.m_uiInstanceCount             = built.m_uiInstanceCount;
    description.m_uiRayEmissivePrimitiveCount = built.m_uiEmissivePrimitiveCount;
    const auto emissive                       = xiiEmissiveLightManager::AddBuildPass(context.m_Graph, context.m_Graph.GetFrameIndex(), description);
    if (!emissive.IsValid())
      return context.ReportFailure("Ray-scene emissive proposal tables could not be built.");
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_EmissiveTriangles, emissive.m_hTriangles));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_EmissiveHeader, emissive.m_hHeader));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_TriangleAliases, emissive.m_hTriangleAliases));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_BlockAliases, emissive.m_hBlockAliases));
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_AliasHeader, emissive.m_hAliasHeader));
  }
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputScene, {built.m_pTopLevelAS, built.m_hSceneDependency}));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_OutputMaterials, built.m_hMaterialData));
  return context.SetOutput(m_OutputGeometry, built.m_hGeometryData);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_RayTracing_RayTracingScenePass);
