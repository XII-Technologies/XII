/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Pipeline/View.h>
#include <SharedPluginScene/Scene/SceneCameraSettings.h>

xiiResult xiiConfigureSceneCamera(const xiiCameraComponent& component, xiiCamera& inout_camera)
{
  if (!component.IsActiveAndInitialized() || !component.GetOwner())
    return XII_FAILURE;
  xiiCameraMode::Enum mode      = component.GetCameraMode();
  float               dimension = component.GetFieldOfView();
  if (mode == xiiCameraMode::OrthoFixedHeight || mode == xiiCameraMode::OrthoFixedWidth)
    dimension = component.GetOrthoDimension();
  else if (mode == xiiCameraMode::Stereo)
  {
    mode      = xiiCameraMode::PerspectiveFixedFovY;
    dimension = 45.0f;
  }
  else if (mode != xiiCameraMode::PerspectiveFixedFovX && mode != xiiCameraMode::PerspectiveFixedFovY)
    return XII_FAILURE;

  const float nearPlane = component.GetNearPlane();
  const float farPlane  = xiiMath::Max(nearPlane + 0.00001f, component.GetFarPlane());
  const float exposure  = component.GetExposure();
  if (!xiiMath::IsFinite(dimension) || dimension <= 0 || !xiiMath::IsFinite(nearPlane) || !xiiMath::IsFinite(component.GetFarPlane()) || !xiiMath::IsFinite(farPlane) || farPlane <= nearPlane || !xiiMath::IsFinite(exposure) || exposure <= 0 || ((mode == xiiCameraMode::PerspectiveFixedFovX || mode == xiiCameraMode::PerspectiveFixedFovY) && (dimension >= 180 || nearPlane <= 0)))
    return XII_FAILURE;

  const auto* owner    = component.GetOwner();
  const auto  position = owner->GetGlobalPosition();
  const auto  forward  = owner->GetGlobalDirForwards();
  const auto  up       = owner->GetGlobalDirUp();
  if (!position.IsValid() || !forward.IsValid() || !up.IsValid())
    return XII_FAILURE;
  inout_camera.SetCameraMode(mode, dimension, nearPlane, farPlane);
  inout_camera.LookAt(position, position + forward, up);
  inout_camera.SetExposure(exposure);
  return XII_SUCCESS;
}

xiiResult xiiConfigureSceneCullingCamera(const xiiView& view, bool bUseComponent, xiiCamera& inout_camera)
{
  if (!view.GetWorld() || !view.GetCamera() || view.GetCamera()->GetCameraMode() == xiiCameraMode::None || &inout_camera == view.GetCamera())
    return XII_FAILURE;
  inout_camera = *view.GetCamera();
  if (bUseComponent)
    if (const auto* manager = view.GetWorld()->GetComponentManager<xiiCameraComponentManager>())
      if (const auto* component = manager->GetCameraByUsageHint(xiiCameraUsageHint::CullingOnly))
        return xiiConfigureSceneCamera(*component, inout_camera);
  return XII_SUCCESS;
}
