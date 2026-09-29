/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Components/Lights/ReflectionCaptureComponent.h>
#include <Shaders/Pipeline/ReflectionProbeData.h>

XII_CREATE_SIMPLE_TEST(Lighting, ReflectionProbes)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GPU layout remains shader-compatible")
  {
    XII_TEST_INT(sizeof(xiiGPUReflectionProbe), 128U);
    XII_TEST_INT(sizeof(xiiReflectionProbeConstants), 16U);
    XII_TEST_INT(XII_MAX_REFLECTION_PROBES, 64U);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Authoring type is reflected")
  {
    const xiiRTTI* pComponentType = xiiGetStaticRTTI<xiiReflectionCaptureComponent>();
    XII_TEST_BOOL(pComponentType->FindPropertyByName("ReflectionMap") != nullptr);
    XII_TEST_BOOL(pComponentType->FindPropertyByName("InfluenceShape") != nullptr);
    XII_TEST_BOOL(pComponentType->FindPropertyByName("HalfExtents") != nullptr);
    XII_TEST_BOOL(pComponentType->FindPropertyByName("SphereRadius") != nullptr);
    XII_TEST_BOOL(pComponentType->FindPropertyByName("BlendDistance") != nullptr);
    XII_TEST_BOOL(pComponentType->FindPropertyByName("Intensity") != nullptr);
    XII_TEST_BOOL(pComponentType->FindPropertyByName("Saturation") != nullptr);
    XII_TEST_BOOL(pComponentType->FindPropertyByName("Priority") != nullptr);
    XII_TEST_BOOL(pComponentType->FindPropertyByName("ParallaxCorrected") != nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid influence values are contained")
  {
    xiiReflectionCaptureComponent probe;
    probe.SetHalfExtents(xiiVec3(-1.0f, 0.0f, 2.0f));
    probe.SetSphereRadius(-4.0f);
    probe.SetBlendDistance(-2.0f);
    probe.SetIntensity(-1.0f);
    probe.SetSaturation(-1.0f);

    XII_TEST_VEC3(probe.GetHalfExtents(), xiiVec3(0.01f, 0.01f, 2.0f), 0.0001f);
    XII_TEST_FLOAT(probe.GetSphereRadius(), 0.01f, 0.0001f);
    XII_TEST_FLOAT(probe.GetBlendDistance(), 0.0f, 0.0f);
    XII_TEST_FLOAT(probe.GetIntensity(), 0.0f, 0.0f);
    XII_TEST_FLOAT(probe.GetSaturation(), 0.0f, 0.0f);
  }
}

