/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/Passes/Output/BloomPass.h>
#include <GraphicsCore/Pipeline/Passes/Output/ColorGradingPass.h>
#include <GraphicsCore/Pipeline/Passes/Output/ToneMappingPass.h>
#include <GraphicsCore/Pipeline/Passes/Output/UpscalePass.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

XII_CREATE_SIMPLE_TEST(Pipeline, OutputOperationComposition)
{
  xiiUpscalePass authored;
  authored.m_OutputUpscaledColor.SetResolution({xiiRenderPipelineTextureResolutionMode::Fixed, 128, 64});
  xiiVariantDictionary properties;
  XII_TEST_RESULT(authored.GetProperties(properties));
  xiiUpscalePass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  XII_TEST_RESULT(copy.GetConfiguredPins(pins));
  XII_TEST_INT(pins[1].m_Resolution.m_uiWidth, 128);
  XII_TEST_INT(pins[1].m_Resolution.m_uiHeight, 64);
  copy.m_OutputUpscaledColor.SetResolution({xiiRenderPipelineTextureResolutionMode::Fixed, 65537, 64});
  XII_TEST_BOOL(copy.GetConfiguredPins(pins).Failed());
  xiiBloomPass bloom;
  properties.Clear();
  properties.Insert("Radius", -1.0f);
  XII_TEST_BOOL(bloom.Configure(properties).Failed());

  xiiRenderPipelineResourceDescriptor child;
  child.AddPass("Spatial upscale", xiiGetStaticRTTI<xiiUpscalePass>());
  child.m_Interface.PushBack({"Input", true, child.m_Passes[0].m_Id, "TAAResolved"});
  child.m_Interface.PushBack({"Output", false, child.m_Passes[0].m_Id, "UpscaledColor"});
  xiiGraphDefinition definition;
  XII_TEST_RESULT(child.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("First", definition.m_Id);
  root.AddSubgraph("Second", definition.m_Id);
  root.m_Interface.PushBack({"Input", true, root.m_Passes[0].m_Id, "Input"});
  root.m_Interface.PushBack({"Input", true, root.m_Passes[1].m_Id, "Input"});
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
  XII_TEST_INT(compiled->GetExecutionOrder().GetCount(), 2);
  XII_TEST_BOOL(compiled->GetDescriptor().m_Settings.m_RequiredDeviceFeatures.IsSet(xiiRenderPipelineDeviceFeature::Compute));
  XII_TEST_BOOL(compiled->GetExtractionRequirements().IsNoFlagSet());
  XII_TEST_BOOL(compiled->GetRuntimeFeatures().IsNoFlagSet());
  XII_TEST_BOOL(compiled->GetPins(0)[0].m_Id != compiled->GetPins(1)[0].m_Id);
}
