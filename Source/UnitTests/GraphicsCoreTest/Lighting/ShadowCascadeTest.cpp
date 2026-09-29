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
  settings.m_uiCascadeCount = 4U;
  settings.m_fMaximumShadowDistance = 300.0f;
  settings.m_fSplitLambda = 0.7f;
  settings.m_uiShadowMapResolution = 2048U;

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
    fPreviousFar = cascade.m_fSplitFar;
  }
  XII_TEST_FLOAT(cascades.PeekBack().m_fSplitFar, settings.m_fMaximumShadowDistance, 0.01f);
}
