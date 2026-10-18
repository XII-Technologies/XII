/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/Passes/Composition/ConfidenceRadiancePass.h>
#include <GraphicsCore/Pipeline/Passes/Composition/RadianceAddPass.h>
#include <GraphicsCore/Pipeline/Passes/Composition/VolumetricCompositePass.h>
#include <GraphicsCore/Pipeline/Passes/Reflections/ScreenSpaceReflectionsPass.h>
#include <GraphicsCore/Pipeline/Passes/Reflections/ScreenSpaceReflectionsTemporalPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearTexturePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

XII_CREATE_SIMPLE_TEST(Pipeline, AuthoredOpaqueComposition)
{
  xiiScreenSpaceReflectionsPass trace;
  trace.m_uiMaxSteps = 256;
  trace.m_fThickness = 0.01f;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(trace.GetProperties(properties));
  xiiScreenSpaceReflectionsPass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_INT(copy.m_uiMaxSteps, 256);
  XII_TEST_FLOAT(copy.m_fThickness, 0.01f, 0);
  properties.Insert("MaxSteps", 0U);
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  xiiScreenSpaceReflectionsTemporalPass temporal;
  properties.Clear();
  properties.Insert("HistoryWeight", 1.1f);
  XII_TEST_BOOL(temporal.Configure(properties).Failed());
  xiiClearTexturePass clear;
  properties.Clear();
  properties.Insert("Format", static_cast<xiiInt64>(xiiGALResourceFormat::D32Float));
  XII_TEST_BOOL(clear.Configure(properties).Failed());

  const auto                                 resource = xiiLoadPipelineLibraryResource("OpaqueComposition");
  xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  const auto descriptor = loaded->GetAuthoring()->m_Descriptor;
  XII_TEST_INT(descriptor.m_Passes.GetCount(), 8);
  XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet());
  XII_TEST_BOOL(loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
  xiiDynamicArray<xiiRenderPipelinePin> interfacePins;
  XII_TEST_RESULT(xiiGetRenderPipelineInterface(descriptor, interfacePins));
  for (const auto& pin : interfacePins)
    XII_TEST_BOOL(pin.m_sName != "PlanarReflectionMap" && (pin.m_bInput || (pin.m_sName != "DirectLighting" && pin.m_sName != "IndirectLighting" && pin.m_sName != "RTFinalGI" && pin.m_sName != "GBufferEmissive" && pin.m_sName != "VolumetricScattering")));
  xiiGraphDefinition definition;
  XII_TEST_RESULT(descriptor.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.m_References = descriptor.m_References;
  root.AddSubgraph("First", definition.m_Id);
  root.AddSubgraph("Second", definition.m_Id);
  for (const auto& pin : interfacePins)
    if (pin.m_bInput)
    {
      root.m_Interface.PushBack({pin.m_sName, true, root.m_Passes[0].m_Id, pin.m_sName});
      root.m_Interface.PushBack({pin.m_sName, true, root.m_Passes[1].m_Id, pin.m_sName});
    }
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
  if (compiled)
  {
    XII_TEST_INT(compiled->GetExecutionOrder().GetCount(), 24);
    XII_TEST_BOOL(compiled->GetProvenance()[0].m_InstancePath != compiled->GetProvenance()[12].m_InstancePath);
  }
}

XII_CREATE_SIMPLE_TEST(Pipeline, IndependentRadianceCompositionSchemas)
{
  xiiRadianceAddPass sum;
  sum.m_fScaleA = 0.25f;
  sum.m_fScaleB = 2;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(sum.GetProperties(properties));
  XII_TEST_BOOL(!properties.Contains("A") && !properties.Contains("Radiance"));
  xiiRadianceAddPass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_FLOAT(copy.m_fScaleA, 0.25f, 0);
  XII_TEST_FLOAT(copy.m_fScaleB, 2, 0);
  properties.Clear();
  properties.Insert("ScaleA", -1.0f);
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  xiiConfidenceRadiancePass             confidence;
  xiiVolumetricCompositePass            fog;
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  sum.GetPins(pins);
  XII_TEST_RESULT(sum.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[sum.m_Output.GetBindingIndex()].m_Format == xiiGALResourceFormat::RGBA32Float);
  pins.Clear();
  confidence.GetPins(pins);
  XII_TEST_RESULT(confidence.InitializePinBindings(pins));
  XII_TEST_BOOL(!pins[confidence.m_Signal.GetBindingIndex()].m_bOptional);
  pins.Clear();
  fog.GetPins(pins);
  XII_TEST_RESULT(fog.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[fog.m_Output.GetBindingIndex()].m_Format == xiiGALResourceFormat::RGBA16Float);
  XII_TEST_BOOL(sum.GetRuntimeFeatures().IsNoFlagSet() && confidence.GetRuntimeFeatures().IsNoFlagSet() && fog.GetRuntimeFeatures().IsNoFlagSet());
  XII_TEST_BOOL(sum.GetExtractionRequirements().IsNoFlagSet() && confidence.GetExtractionRequirements().IsNoFlagSet() && fog.GetExtractionRequirements().IsNoFlagSet());
}
