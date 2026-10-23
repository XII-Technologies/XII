/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowCascadePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>

namespace
{
  bool ValidCascadeProperties(const xiiShadowCascadeSettings& settings, const xiiVec3& direction)
  {
    return settings.m_uiCascadeCount >= 1 && settings.m_uiCascadeCount <= 4 &&
      xiiMath::IsFinite(settings.m_fSplitLambda) && settings.m_fSplitLambda >= 0 && settings.m_fSplitLambda <= 1 &&
      xiiMath::IsFinite(settings.m_fMaximumShadowDistance) && settings.m_fMaximumShadowDistance > 0 &&
      xiiMath::IsFinite(settings.m_fDepthPadding) && settings.m_fDepthPadding >= 0 &&
      settings.m_uiShadowMapResolution >= 64 && settings.m_uiShadowMapResolution <= 16384 &&
      direction.IsValid() && !direction.IsZero();
  }
} // namespace
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShadowCascadePass, 1, xiiRTTIDefaultAllocator<xiiShadowCascadePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("CascadeConstants", m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("CascadeSnapshot", m_Snapshot)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Settings", m_Settings),
    XII_MEMBER_PROPERTY("LightDirection", m_LightDirection)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0, 0, -1))),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Shadows") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiShadowCascadePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return ValidCascadeProperties(m_Settings, m_LightDirection) ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiShadowCascadePass::Build(xiiRenderPipelinePassContext& context)
{
  auto direction = m_LightDirection;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "ShadowLightDirection", direction));
  if (!ValidCascadeProperties(m_Settings, direction))
    return context.ReportFailure("Invalid directional cascade settings or light direction.");
  if (!context.m_pView || !context.m_pView->GetCamera())
    return context.ReportFailure("Directional cascades require an explicit view camera.");
  const auto& camera   = *context.m_pView->GetCamera();
  const auto& viewport = context.m_pView->GetViewport();
  if (camera.IsStereoscopic() || (!camera.IsPerspective() && !camera.IsOrthographic()) || !xiiMath::IsFinite(viewport.width) || !xiiMath::IsFinite(viewport.height) || viewport.width <= 0 || viewport.height <= 0 || !xiiMath::IsFinite(camera.GetNearPlane()) || !xiiMath::IsFinite(camera.GetFarPlane()) || camera.GetNearPlane() <= 0 || camera.GetFarPlane() <= camera.GetNearPlane() || !xiiMath::IsFinite(camera.GetFovOrDim()) || camera.GetFovOrDim() <= 0 || (camera.IsPerspective() && camera.GetFovOrDim() >= 180) || !camera.GetPosition().IsValid() || !camera.GetDirForwards().IsValid() || !camera.GetDirRight().IsValid() || !camera.GetDirUp().IsValid())
    return context.ReportFailure("Directional cascades require a finite perspective/orthographic camera and nonempty viewport; stereo requires an explicit eye view.");
  xiiStaticArray<xiiShadowCascadeDescription, 4> cascades;
  if (xiiShadowCascadeUtils::Build(camera, viewport.width / viewport.height, direction, m_Settings, cascades).Failed())
    return context.ReportFailure("Directional cascade construction failed for the supplied camera and settings.");
  for (const auto& cascade : cascades)
    if (!cascade.m_mViewProjection.IsValid() || !xiiMath::IsFinite(cascade.m_fSplitFar) || !xiiMath::IsFinite(cascade.m_fWorldRadius) || cascade.m_fWorldRadius <= 0)
      return context.ReportFailure("Directional cascade construction produced nonfinite projection data.");
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle m_Buffer;
    xiiShadowCascadeConstants  m_Snapshot;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Buffer = xiiRenderPassUtils::Constants(builder, "Directional Cascade Constants", sizeof(data.m_Snapshot));
    for (auto& matrix : data.m_Snapshot.CascadeViewProjection) matrix = xiiMat4::MakeIdentity();
    data.m_Snapshot.CascadeSplitDepths = xiiVec4::MakeZero();
    data.m_Snapshot.CascadeWorldRadii = xiiVec4::MakeZero();
    data.m_Snapshot.ActiveCascadeCount = cascades.GetCount();
    data.m_Snapshot._Pad = xiiVec3::MakeZero();
    for (xiiUInt32 i = 0; i < cascades.GetCount(); ++i)
    {
      data.m_Snapshot.CascadeViewProjection[i] = cascades[i].m_mViewProjection;
      data.m_Snapshot.CascadeSplitDepths.GetData()[i] = cascades[i].m_fSplitFar;
      data.m_Snapshot.CascadeWorldRadii.GetData()[i] = cascades[i].m_fWorldRadius;
    } }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    xiiGALMapHelper<xiiShadowCascadeConstants> mapped(execute.GetCommandList(), execute.GetBuffer(data.m_Buffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);
    *mapped = data.m_Snapshot; });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Constants, emitted.first->m_Buffer));
  return context.SetOutput(m_Snapshot, XII_DEFAULT_NEW(xiiShadowCascadeSnapshot, context.m_Graph.GetSetupSerial(), cascades, m_Settings.m_uiShadowMapResolution, emitted.first->m_Buffer));
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Shadows_ShadowCascadePass);
