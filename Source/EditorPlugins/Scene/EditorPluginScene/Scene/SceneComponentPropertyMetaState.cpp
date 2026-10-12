/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Scene/SceneComponentPropertyMetaState.h>
#include <GraphicsCore/Components/Lights/LightComponent.h>
#include <GraphicsCore/Components/Lights/ReflectionCaptureComponent.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

void xiiCameraComponent_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  if (!e.m_pObject->GetType()->IsDerivedFrom<xiiCameraComponent>())
    return;
  const auto usage                              = e.m_pObject->GetTypeAccessor().GetValue("UsageHint").ConvertTo<xiiInt64>();
  const bool isRenderTarget                     = usage == xiiCameraUsageHint::RenderTarget;
  auto&      properties                         = *e.m_pPropertyStates;
  properties["RenderTargetOffset"].m_Visibility = isRenderTarget ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
  properties["RenderTargetSize"].m_Visibility   = isRenderTarget ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
}

void xiiReflectionCaptureComponent_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  if (!e.m_pObject->GetType()->IsDerivedFrom<xiiReflectionCaptureComponent>())
    return;
  const auto shape                             = e.m_pObject->GetTypeAccessor().GetValue("InfluenceShape").ConvertTo<xiiInt64>();
  auto&      properties                        = *e.m_pPropertyStates;
  properties["HalfExtents"].m_Visibility       = shape == xiiReflectionProbeInfluenceShape::Sphere ? xiiPropertyUiState::Invisible : xiiPropertyUiState::Default;
  properties["SphereRadius"].m_Visibility      = shape == xiiReflectionProbeInfluenceShape::Box ? xiiPropertyUiState::Invisible : xiiPropertyUiState::Default;
  properties["ParallaxCorrected"].m_Visibility = shape == xiiReflectionProbeInfluenceShape::Sphere ? xiiPropertyUiState::Invisible : xiiPropertyUiState::Default;
}

void xiiLightComponent_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  const auto* type = e.m_pObject->GetType();
  if (!type->IsDerivedFrom<xiiLightComponent>() || type->FindPropertyByName("ShadowFadeOutRange") == nullptr)
    return;
  const bool castsShadows                                   = e.m_pObject->GetTypeAccessor().GetValue("CastShadows").ConvertTo<bool>();
  (*e.m_pPropertyStates)["ShadowFadeOutRange"].m_Visibility = castsShadows ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
}
