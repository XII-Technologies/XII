/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/WorldInstancesUploadPass.h>
#include <GraphicsCore/Pipeline/Passes/Residency/GeometryUploadPass.h>
#include <GraphicsCore/Pipeline/Passes/Residency/MaterialUploadPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/CounterResetPass.h>
#include <GraphicsCore/Pipeline/Passes/Visibility/VisibilityViewPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

XII_CREATE_SIMPLE_TEST_GROUP(NativePreparation);
XII_CREATE_SIMPLE_TEST(NativePreparation, ReflectedDataProducers)
{
  xiiWorldInstancesUploadPass instances;
  xiiGeometryUploadPass       geometry;
  xiiMaterialUploadPass       materials;
  xiiVisibilityViewPass       visibility;
  for (xiiRenderPipelinePass* pass : {static_cast<xiiRenderPipelinePass*>(&instances), static_cast<xiiRenderPipelinePass*>(&geometry), static_cast<xiiRenderPipelinePass*>(&materials), static_cast<xiiRenderPipelinePass*>(&visibility)})
  {
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    pass->GetPins(pins);
    XII_TEST_RESULT(pass->InitializePinBindings(pins));
    XII_TEST_BOOL(pass->GetDynamicRTTI()->FindPropertyByName("Stage") == nullptr);
    for (const auto& pin : pins)
      if (pin.m_Type == xiiRenderPipelinePin::Buffer)
        XII_TEST_BOOL(!pin.m_bInput && pin.m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::ShaderResource));
  }
  XII_TEST_BOOL(instances.GetExtractionRequirements() == xiiRenderExtractionCategory::Geometry);
  XII_TEST_BOOL(geometry.GetExtractionRequirements().IsNoFlagSet() && materials.GetExtractionRequirements().IsNoFlagSet());
  visibility.m_uiVisibilityMask = 0x42;
  visibility.m_fLodScreenScale  = 2;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(visibility.GetProperties(properties));
  XII_TEST_BOOL(!properties.Contains("VisibilityView") && !properties.Contains("Candidates"));
  xiiVisibilityViewPass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_INT(copy.m_uiVisibilityMask, 0x42);
  XII_TEST_FLOAT(copy.m_fLodScreenScale, 2, 0);
  properties.Insert("MaxVisibleMeshlets", 0U);
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  xiiRenderPipelineResourceDescriptor leaf;
  leaf.AddPass("Instances", "xiiWorldInstancesUploadPass");
  leaf.AddPass("Geometry", "xiiGeometryUploadPass");
  leaf.AddPass("Materials", "xiiMaterialUploadPass");
  xiiGraphDefinition definition;
  XII_TEST_RESULT(leaf.MakeGraphDefinition(definition));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("First", definition.m_Id);
  root.AddSubgraph("Second", definition.m_Id);
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
  if (!compiled)
    return;
  XII_TEST_INT(compiled->GetExecutionOrder().GetCount(), 6);
  XII_TEST_BOOL(compiled->GetExtractionRequirements() == xiiRenderExtractionCategory::Geometry);
  XII_TEST_BOOL(compiled->GetRuntimeFeatures() == xiiRenderPipelineFeature::GpuScene);
  xiiContiguousMemoryStreamStorage bytes;
  xiiMemoryStreamWriter            writer(&bytes);
  XII_TEST_RESULT(root.Serialize(writer));
  xiiMemoryStreamReader               reader(&bytes);
  xiiRenderPipelineResourceDescriptor restored;
  XII_TEST_RESULT(restored.Deserialize(reader));
  xiiSharedPtr<const xiiRenderPipeline> repeated;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(restored, repeated));
  if (repeated)
    for (xiiUInt32 i = 0; i < compiled->GetDescriptor().m_Passes.GetCount(); ++i)
      XII_TEST_BOOL(compiled->GetDescriptor().m_Passes[i].m_Id == repeated->GetDescriptor().m_Passes[i].m_Id);
}

XII_CREATE_SIMPLE_TEST(NativePreparation, IndependentCounterConfiguration)
{
  xiiCounterResetPass counter;
  counter.m_uiValue = 17;
  xiiVariantDictionary properties;
  XII_TEST_RESULT(counter.GetProperties(properties));
  XII_TEST_INT(properties["Value"].Get<xiiUInt32>(), 17);
  XII_TEST_BOOL(!properties.Contains("Counter"));
  xiiCounterResetPass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_INT(copy.m_uiValue, 17);
  XII_TEST_BOOL(copy.GetRuntimeFeatures().IsNoFlagSet() && copy.GetExtractionRequirements().IsNoFlagSet());
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  copy.GetPins(pins);
  XII_TEST_RESULT(copy.InitializePinBindings(pins));
  const auto& pin = pins[copy.m_Counter.GetBindingIndex()];
  XII_TEST_BOOL(!pin.m_bInput && pin.m_Type == xiiRenderPipelinePin::Buffer);
  XII_TEST_BOOL(pin.m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::IndirectDrawArguments));
}
