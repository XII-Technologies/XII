/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/Time/Clock.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/View.h>

xiiFrameCameraConstants xiiSnapshotCameraConstants(const xiiView& view)
{
  xiiFrameCameraConstants result;
  const auto*             camera = view.GetCamera();
  if (!camera)
    return result;
  result.m_ViewProjectionMatrix        = view.GetViewProjectionMatrix(xiiCameraEye::Left);
  result.m_InverseViewProjectionMatrix = view.GetInverseViewProjectionMatrix(xiiCameraEye::Left);
  result.m_CameraPositionAndNearPlane  = xiiVec4(camera->GetPosition(), camera->GetNearPlane());
  auto direction                       = camera->GetDirForwards();
  if (direction.NormalizeIfNotZero(xiiVec3(1, 0, 0)).Failed())
    direction = xiiVec3(1, 0, 0);
  result.m_CameraForwardAndFarPlane = xiiVec4(direction, camera->GetFarPlane());
  return result;
}
xiiFrameGlobalConstants xiiSnapshotGlobalConstants(const xiiView& view, xiiUInt32 frameIndex)
{
  xiiFrameGlobalConstants result;
  result.m_uiFrameIndex = frameIndex;
  if (const auto* clock = xiiClock::GetGlobalClock())
  {
    result.m_fDeltaTimeMs = static_cast<float>(clock->GetTimeDiff().GetMilliseconds());
    result.m_fGlobalTime  = static_cast<float>(clock->GetAccumulatedTime().GetSeconds());
    result.m_fWorldTime   = result.m_fGlobalTime;
  }
  result.m_RenderScaleJitter = xiiVec4(view.GetRenderResolutionScale(), 0, 0, 0);
  return result;
}
