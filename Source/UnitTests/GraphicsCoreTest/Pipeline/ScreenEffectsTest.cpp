/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/ScreenSpaceGlobalIlluminationPass.h>
#include <GraphicsCore/Pipeline/Passes/Refraction/ScreenSpaceRefractionPass.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/DepthSensorPass.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

XII_CREATE_SIMPLE_TEST(Pipeline, ScreenEffectResourceComposition)
{
  xiiScreenSpaceGlobalIlluminationPass gi;
  gi.m_fIntensity = 0.25f;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(gi.GetProperties(properties));
  xiiScreenSpaceGlobalIlluminationPass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_FLOAT(copy.m_fIntensity, 0.25f, 0.0001f);
  properties.Insert("SampleCount", 0U);
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  xiiScreenSpaceRefractionPass refraction;
  properties.Clear();
  properties.Insert("MaximumDistance", -1.0f);
  XII_TEST_BOOL(refraction.Configure(properties).Failed());
  xiiDepthSensorPass sensor;
  sensor.m_Calibration.m_uiResolutionX = 23;
  XII_TEST_RESULT(sensor.GetProperties(properties));
  xiiDepthSensorPass sensorCopy;
  XII_TEST_RESULT(sensorCopy.Configure(properties));
  XII_TEST_INT(sensorCopy.m_Calibration.m_uiResolutionX, 23);
  XII_TEST_BOOL(sensorCopy.GetExtractionRequirements().IsNoFlagSet());
  XII_TEST_BOOL(sensorCopy.GetRuntimeFeatures().IsNoFlagSet());
  xiiAtmosphereConstantsPass atmosphere;
  XII_TEST_BOOL(atmosphere.GetExtractionRequirements().IsNoFlagSet());
  atmosphere.m_bUseWorldAtmosphere = true;
  XII_TEST_BOOL(atmosphere.GetExtractionRequirements() == xiiRenderExtractionCategory::Atmosphere);

  auto                                       childResource = xiiLoadPipelineLibraryResource("ScreenEffects");
  xiiResourceLock<xiiRenderPipelineResource> child(childResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(child.IsValid() && child->GetPipeline()))
    return;
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({child->GetAuthoring()->m_Descriptor.m_Id, childResource.GetResourceID()});
  root.AddSubgraph("First", child->GetAuthoring()->m_Descriptor.m_Id);
  root.AddSubgraph("Second", child->GetAuthoring()->m_Descriptor.m_Id);
  for (const auto& input : child->GetAuthoring()->m_Descriptor.m_Interface)
    if (input.m_bInput)
      for (const auto& occurrence : root.m_Passes)
      {
        bool exists = false;
        for (const auto& boundary : root.m_Interface)
          exists |= boundary.m_sName == input.m_sName && boundary.m_Node == occurrence.m_Id;
        if (!exists)
          root.m_Interface.PushBack({input.m_sName, true, occurrence.m_Id, input.m_sName});
      }
  xiiSharedPtr<const xiiRenderPipeline> compiled, compiledAgain;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiledAgain));
  if (!XII_TEST_BOOL(compiled && compiledAgain))
    return;
  XII_TEST_INT(compiled->GetDescriptor().m_Passes.GetCount(), 2 * child->GetPipeline()->GetDescriptor().m_Passes.GetCount());
  XII_TEST_BOOL(compiled->GetRuntimeFeatures().IsNoFlagSet());
  XII_TEST_BOOL(compiled->GetExtractionRequirements() == xiiRenderExtractionCategory::Atmosphere);
  XII_TEST_BOOL(compiled->GetProvenance()[0].m_InstancePath[0] != compiled->GetProvenance()[child->GetPipeline()->GetDescriptor().m_Passes.GetCount()].m_InstancePath[0]);
  for (xiiUInt32 index = 0; index < compiled->GetDescriptor().m_Passes.GetCount(); ++index)
    XII_TEST_BOOL(compiled->GetDescriptor().m_Passes[index].m_Id == compiledAgain->GetDescriptor().m_Passes[index].m_Id);
}
