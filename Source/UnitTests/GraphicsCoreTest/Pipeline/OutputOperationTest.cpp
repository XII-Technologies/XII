/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/BuiltinRenderPipelinePass.h>
#include <GraphicsCore/Pipeline/Passes/Output/BloomPass.h>
#include <GraphicsCore/Pipeline/Passes/Output/ColorGradingPass.h>
#include <GraphicsCore/Pipeline/Passes/Output/ToneMappingPass.h>
#include <GraphicsCore/Pipeline/Passes/Output/UpscalePass.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

XII_CREATE_SIMPLE_TEST(Pipeline, OutputOperationComposition)
{
  const char* operations[] = {"Upscale", "Bloom", "ToneMapping", "ColorGrading"};
  const char* types[]      = {"xiiUpscalePass", "xiiBloomPass", "xiiToneMappingPass", "xiiColorGradingPass"};
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(operations); ++i)
  {
    xiiRenderPipelineResourceDescriptor legacy;
    xiiVariantDictionary                properties;
    properties.Insert("Stage", operations[i]);
    const auto                       index = legacy.AddPass("Authored operation", "xiiBuiltinRenderPipelinePass", properties);
    const auto                       id    = legacy.m_Passes[index].m_Id;
    xiiContiguousMemoryStreamStorage bytes;
    xiiMemoryStreamWriter            writer(&bytes);
    XII_TEST_RESULT(legacy.Serialize(writer));
    xiiMemoryStreamReader               reader(&bytes);
    xiiRenderPipelineResourceDescriptor migrated;
    XII_TEST_RESULT(migrated.Deserialize(reader));
    XII_TEST_BOOL(migrated.m_Passes[index].m_sType == types[i]);
    XII_TEST_BOOL(migrated.m_Passes[index].m_Id == id);
    XII_TEST_BOOL(!migrated.m_Passes[index].m_Properties.Contains("Stage"));
    xiiBuiltinRenderPipelinePass removed;
    XII_TEST_BOOL(removed.Configure(properties).Failed());
  }
  xiiUpscalePass authored;
  authored.m_uiWidth  = 128;
  authored.m_uiHeight = 64;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(authored.GetProperties(properties));
  xiiUpscalePass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_INT(copy.m_uiWidth, 128);
  XII_TEST_INT(copy.m_uiHeight, 64);
  properties.Insert("Width", 65537U);
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  xiiBloomPass bloom;
  properties.Clear();
  properties.Insert("Radius", -1.0f);
  XII_TEST_BOOL(bloom.Configure(properties).Failed());

  xiiRenderPipelineResourceDescriptor child;
  child.AddPass("Spatial upscale", "xiiUpscalePass");
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
  XII_TEST_BOOL(compiled->GetPins(0)[2].m_Id != compiled->GetPins(1)[2].m_Id);

  xiiRenderPipelineResourceDescriptor sdk;
  XII_TEST_RESULT(xiiLoadPipelineLibraryDescriptor(sdk));
  xiiUInt32 migrated = 0;
  for (const auto& pass : sdk.m_Passes)
    for (const auto* type : types)
      migrated += pass.m_sType == type ? 1 : 0;
  XII_TEST_INT(migrated, 4);
}
