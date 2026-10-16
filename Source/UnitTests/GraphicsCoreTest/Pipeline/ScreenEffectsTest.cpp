/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/ScreenSpaceGlobalIlluminationPass.h>
#include <GraphicsCore/Pipeline/Passes/Refraction/ScreenSpaceRefractionPass.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/SensorOutputPass.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

XII_CREATE_SIMPLE_TEST(Pipeline, ScreenEffectResourceMigration)
{
  xiiRenderPipelineResourceDescriptor legacy;
  for (xiiStringView name : {"Visibility", "Lighting", "ScreenEffects"})
  {
    xiiVariantDictionary properties;
    properties.Insert("Stage", xiiString(name));
    legacy.AddPass(name, "xiiBuiltinRenderPipelinePass", properties);
  }
  const auto screenId = legacy.m_Passes[2].m_Id;
  legacy.m_Passes[2].m_Parameters.Insert("Custom", 17U);
  xiiGraphDefinition nested;
  XII_TEST_RESULT(legacy.MakeGraphDefinition(nested));
  legacy.m_Definitions.PushBack(nested);
  // An authored connection takes precedence over migration's formerly implicit source.
  XII_TEST_RESULT(legacy.Connect(1, "PerFrameCamera", 2, "PerFrameCamera"));
  xiiContiguousMemoryStreamStorage bytes;
  xiiMemoryStreamWriter            writer(&bytes);
  XII_TEST_RESULT(legacy.Serialize(writer));
  xiiMemoryStreamReader               reader(&bytes);
  xiiRenderPipelineResourceDescriptor converted;
  XII_TEST_RESULT(converted.Deserialize(reader));
  XII_TEST_BOOL(converted.m_Passes[2].m_Id == screenId);
  XII_TEST_BOOL(converted.m_Passes[2].m_sType.IsEmpty());
  XII_TEST_BOOL(converted.m_Passes[2].m_Properties.IsEmpty());
  XII_TEST_BOOL(converted.m_Passes[2].m_Parameters == legacy.m_Passes[2].m_Parameters);
  XII_TEST_INT(converted.m_References.GetCount(), 1);
  XII_TEST_BOOL(converted.m_References[0].m_Id == converted.m_Passes[2].m_Definition);
  XII_TEST_BOOL(converted.m_References[0].m_sResource == "RenderPipelines/ScreenEffects.xiiBinRenderPipeline");
  XII_TEST_INT(converted.m_Connections.GetCount(), 4);
  XII_TEST_INT(converted.m_Connections[0].m_uiSource, 1);
  XII_TEST_BOOL(converted.m_Connections[0].m_sInput == "PerFrameCamera");
  XII_TEST_BOOL(converted.m_Definitions[0].m_Nodes[2].m_Id == screenId);
  XII_TEST_BOOL(converted.m_Definitions[0].m_Nodes[2].m_Definition == converted.m_Passes[2].m_Definition);
  XII_TEST_INT(converted.m_Definitions[0].m_Edges.GetCount(), 4);
  XII_TEST_RESULT(converted.m_Definitions[0].Validate());
  xiiContiguousMemoryStreamStorage first, second;
  xiiMemoryStreamWriter            firstWriter(&first), secondWriter(&second);
  XII_TEST_RESULT(converted.Serialize(firstWriter));
  xiiMemoryStreamReader               firstReader(&first);
  xiiRenderPipelineResourceDescriptor repeated;
  XII_TEST_RESULT(repeated.Deserialize(firstReader));
  XII_TEST_RESULT(repeated.Serialize(secondWriter));
  XII_TEST_BOOL(xiiMakeArrayPtr(first.GetData(), first.GetStorageSize32()) == xiiMakeArrayPtr(second.GetData(), second.GetStorageSize32()));

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
  xiiSensorOutputPass sensor;
  sensor.m_Profile.m_uiResolutionX = 23;
  XII_TEST_RESULT(sensor.GetProperties(properties));
  xiiSensorOutputPass sensorCopy;
  XII_TEST_RESULT(sensorCopy.Configure(properties));
  XII_TEST_INT(sensorCopy.m_Profile.m_uiResolutionX, 23);
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
  for (const auto& pass : child->GetPipeline()->GetDescriptor().m_Passes)
    XII_TEST_BOOL(pass.m_sType != "xiiBuiltinRenderPipelinePass");
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
