/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereApplyPass.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/ScreenIrradianceTracePass.h>
#include <GraphicsCore/Pipeline/Passes/Refraction/ScreenRefractionTracePass.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/DepthSensorPass.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/RgbSensorPass.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

XII_CREATE_SIMPLE_TEST(Pipeline, ScreenEffectResourceComposition)
{
  xiiScreenIrradianceTracePass gi;
  gi.m_uiStepCount = 3;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(gi.GetProperties(properties));
  xiiScreenIrradianceTracePass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_INT(copy.m_uiStepCount, 3);
  properties.Insert("SampleCount", 0U);
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  xiiScreenRefractionTracePass refraction;
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

XII_CREATE_SIMPLE_TEST(Pipeline, IndependentScreenTransportSchemas)
{
  xiiScreenIrradianceTracePass trace;
  xiiScreenRefractionTracePass refract;
  xiiAtmosphereApplyPass       atmosphere;
  xiiRgbSensorPass             rgb;
  xiiVariantDictionary         properties;
  XII_TEST_RESULT(trace.GetProperties(properties));
  XII_TEST_BOOL(!properties.Contains("HardwareGI") && !properties.Contains("SkipWithHardwareGI") && properties.Contains("StepCount"));
  properties.Insert("StepCount", 65U);
  XII_TEST_BOOL(trace.Configure(properties).Failed());
  properties.Clear();
  properties.Insert("Thickness", 0.0f);
  XII_TEST_BOOL(trace.Configure(properties).Failed());
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  trace.GetPins(pins);
  XII_TEST_RESULT(trace.InitializePinBindings(pins));
  for (const auto& pin : pins)
    XII_TEST_BOOL(pin.m_sName != "Albedo" && pin.m_sName != "Material" && pin.m_sName != "PerFrameGlobal");
  XII_TEST_BOOL(pins[trace.m_Output.GetBindingIndex()].m_Format == xiiGALResourceFormat::RGBA32Float);
  pins.Clear();
  refract.GetPins(pins);
  XII_TEST_RESULT(refract.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[refract.m_Radiance.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::ShaderResource) && !pins[refract.m_Radiance.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::UnorderedAccess));
  for (const auto& pin : pins)
    XII_TEST_BOOL(pin.m_sName != "HDRSceneColor" && pin.m_sName != "RefractionSceneColorInput");
  pins.Clear();
  atmosphere.GetPins(pins);
  XII_TEST_RESULT(atmosphere.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[atmosphere.m_Output.GetBindingIndex()].m_Format == xiiGALResourceFormat::RGBA32Float);
  pins.Clear();
  rgb.GetPins(pins);
  XII_TEST_RESULT(rgb.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[rgb.m_Radiance.GetBindingIndex()].m_Format == xiiGALResourceFormat::Unknown);
  XII_TEST_BOOL(trace.GetRuntimeFeatures().IsNoFlagSet() && refract.GetRuntimeFeatures().IsNoFlagSet() && atmosphere.GetRuntimeFeatures().IsNoFlagSet());
  XII_TEST_BOOL(trace.GetExtractionRequirements().IsNoFlagSet() && refract.GetExtractionRequirements().IsNoFlagSet() && atmosphere.GetExtractionRequirements().IsNoFlagSet());
}
