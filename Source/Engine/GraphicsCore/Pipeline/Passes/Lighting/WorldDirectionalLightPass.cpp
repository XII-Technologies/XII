/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/World.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/WorldDirectionalLightPass.h>
#include <GraphicsCore/Pipeline/View.h>
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDirectionalLightFrameData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiWorldDirectionalLightPass, 1, xiiRTTIDefaultAllocator<xiiWorldDirectionalLightPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Before", m_Before)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("After", m_After)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("DirectionalLight", m_Light)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiWorldDirectionalLightPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView || !context.m_pView->GetWorld() || !context.m_pView->GetExtractedRenderData())
    return context.ReportFailure("Directional light selection requires an extracted authoritative view.");
  const xiiWorld&                      world    = *context.m_pView->GetWorld();
  const xiiDirectionalLightRenderData* selected = nullptr;
  for (const auto* packet : context.m_pView->GetExtractedRenderData()->GetAllRenderData())
  {
    const auto* light = xiiDynamicCast<const xiiDirectionalLightRenderData*>(packet);
    if (!light)
      continue;
    const xiiComponent* component = nullptr;
    if (light->m_hOwnerComponent.GetInternalID().m_WorldIndex != world.GetIndex() || !world.TryGetComponent(light->m_hOwnerComponent, component) || !component->IsActiveAndInitialized() || !component->GetOwner()->IsActive() || component->GetOwner()->GetHandle() != light->m_hOwnerObject)
      continue;
    if (!light->m_vDirection.IsValid() || light->m_vDirection.GetLengthSquared() < 1e-12f || !xiiMath::IsFinite(light->m_fPhotometricIntensity) || light->m_fPhotometricIntensity < 0 || !xiiMath::IsFinite(light->m_fRadius) || light->m_fRadius < 0 || light->m_fRadius > 1)
      return context.ReportFailure("Invalid extracted directional emitter.");
    if (!selected || light->m_fPhotometricIntensity > selected->m_fPhotometricIntensity || (light->m_fPhotometricIntensity == selected->m_fPhotometricIntensity && light->m_hOwnerComponent.GetInternalID().m_Data < selected->m_hOwnerComponent.GetInternalID().m_Data))
      selected = light;
  }
  xiiVec4 direction(0, 0, -1, 0), color(1, 1, 1, 0);
  if (selected)
  {
    auto normalized = selected->m_vDirection;
    normalized.NormalizeIfNotZero(xiiVec3(0, 0, -1)).AssertSuccess();
    direction            = xiiVec4(normalized, selected->m_fPhotometricIntensity);
    const auto evaluated = xiiEvaluateLightColor(selected->m_LightColor, selected->m_uiTemperature);
    color                = xiiVec4(evaluated.r, evaluated.g, evaluated.b, selected->m_fRadius);
  }
  return context.SetOutput(m_Light, XII_DEFAULT_NEW(xiiDirectionalLightFrameData, context.m_Graph.GetSetupSerial(), direction, color));
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_WorldDirectionalLightPass);
