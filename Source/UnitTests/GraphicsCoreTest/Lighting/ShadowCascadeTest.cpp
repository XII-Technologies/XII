/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <GraphicsCore/Lighting/ShadowCascade.h>

XII_CREATE_SIMPLE_TEST(Lighting, ShadowCascades)
{
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 70.0f, 0.1f, 1000.0f);
  camera.LookAt(xiiVec3(10.0f, -4.0f, 2.0f), xiiVec3(11.0f, -4.0f, 2.0f), xiiVec3::MakeAxisZ());

  xiiShadowCascadeSettings settings;
  settings.m_uiCascadeCount         = 4U;
  settings.m_fMaximumShadowDistance = 300.0f;
  settings.m_fSplitLambda           = 0.7f;
  settings.m_uiShadowMapResolution  = 2048U;

  xiiStaticArray<xiiShadowCascadeDescription, 4> cascades;
  XII_TEST_BOOL(xiiShadowCascadeUtils::Build(camera, 16.0f / 9.0f, xiiVec3(-0.5f, 0.25f, -1.0f), settings, cascades).Succeeded());
  XII_TEST_INT(cascades.GetCount(), 4);

  float fPreviousFar = camera.GetNearPlane();
  for (const xiiShadowCascadeDescription& cascade : cascades)
  {
    XII_TEST_BOOL(cascade.m_mViewProjection.IsValid());
    XII_TEST_BOOL(!cascade.m_mViewProjection.IsIdentity());
    XII_TEST_FLOAT(cascade.m_fSplitNear, fPreviousFar, 0.001f);
    XII_TEST_BOOL(cascade.m_fSplitFar > cascade.m_fSplitNear);
    XII_TEST_BOOL(cascade.m_fWorldRadius > 0.0f);
    fPreviousFar = cascade.m_fSplitFar;
  }
  XII_TEST_FLOAT(cascades.PeekBack().m_fSplitFar, settings.m_fMaximumShadowDistance, 0.01f);

  // The next projection must cover the whole transition before the logical split,
  // including the corners at the previous cascade's blend start.
  for (xiiUInt32 i = 1; i < cascades.GetCount(); ++i)
  {
    const auto& previous   = cascades[i - 1];
    const float distance   = xiiMath::Lerp(previous.m_fSplitFar, previous.m_fSplitNear, settings.m_fCascadeBlendFraction);
    const float halfHeight = distance * xiiMath::Tan(camera.GetFovY(16.0f / 9.0f) * 0.5f);
    const float halfWidth  = halfHeight * 16.0f / 9.0f;
    for (float x : {-1.0f, 1.0f})
      for (float y : {-1.0f, 1.0f})
      {
        const auto corner = camera.GetPosition() + camera.GetDirForwards() * distance + camera.GetDirRight() * (x * halfWidth) + camera.GetDirUp() * (y * halfHeight);
        const auto clip   = cascades[i].m_mViewProjection.TransformPosition(corner);
        XII_TEST_BOOL(xiiMath::Abs(clip.x) <= 1 && xiiMath::Abs(clip.y) <= 1 && clip.z >= 0 && clip.z <= 1);
      }
  }

  settings.m_fCascadeBlendFraction = -0.1f;
  XII_TEST_BOOL(xiiShadowCascadeUtils::Build(camera, 1, xiiVec3(0, 0, -1), settings, cascades).Failed());
  XII_TEST_BOOL(cascades.IsEmpty());
  settings.m_fCascadeBlendFraction = 0.1f;
  settings.m_fFilterRadius         = xiiMath::NaN<float>();
  XII_TEST_BOOL(xiiShadowCascadeUtils::Build(camera, 1, xiiVec3(0, 0, -1), settings, cascades).Failed());
  settings.m_fFilterRadius = 1;
  for (const xiiVec2 range : {xiiVec2(-10, 30), xiiVec2(-50, -10)})
  {
    camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 8, range.x, range.y);
    XII_TEST_RESULT(xiiShadowCascadeUtils::Build(camera, 16.0f / 9.0f, xiiVec3(-0.5f, 0.25f, -1), settings, cascades));
    XII_TEST_INT(cascades.GetCount(), 4);
    for (xiiUInt32 i = 0; i < cascades.GetCount(); ++i)
    {
      XII_TEST_FLOAT(cascades[i].m_fSplitNear, range.x + float(i) * 10, 0.001f);
      XII_TEST_FLOAT(cascades[i].m_fSplitFar, range.x + float(i + 1) * 10, 0.001f);
      XII_TEST_BOOL(cascades[i].m_mViewProjection.IsValid());
      XII_TEST_BOOL(cascades[i].m_fWorldRadius > 0);
    }
  }
}
