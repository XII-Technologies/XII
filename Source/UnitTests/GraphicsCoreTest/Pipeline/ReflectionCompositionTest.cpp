/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/Pipeline/Passes/Composition/ConfidenceBlendPass.h>
#include <GraphicsCore/Pipeline/Passes/Composition/RadianceReplacePass.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
XII_CREATE_SIMPLE_TEST(Pipeline, AuthoredReflectionComposition)
{
  xiiConfidenceBlendPass blend;
  xiiRadianceReplacePass replace;
  xiiVariantDictionary   properties;
  XII_TEST_RESULT(blend.GetProperties(properties));
  XII_TEST_BOOL(properties.Contains("AsyncCompute") && !properties.Contains("Background"));
  properties.Insert("AsyncCompute", true);
  XII_TEST_RESULT(blend.Configure(properties));
  XII_TEST_BOOL(blend.m_bAsyncCompute);
  properties.Clear();
  properties.Insert("Opacity", 0.25f);
  XII_TEST_RESULT(blend.Configure(properties));
  XII_TEST_FLOAT(blend.m_fOpacity, 0.25f, 0);
  properties.Insert("Opacity", 1.1f);
  XII_TEST_BOOL(blend.Configure(properties).Failed());
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  blend.GetPins(pins);
  XII_TEST_RESULT(blend.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[blend.m_Layer.GetBindingIndex()].m_bOptional);
  pins.Clear();
  replace.GetPins(pins);
  XII_TEST_RESULT(replace.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[replace.m_Scene.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::ShaderResource));
  XII_TEST_BOOL(!pins[replace.m_Scene.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::UnorderedAccess));
  XII_TEST_BOOL(pins[replace.m_Output.GetBindingIndex()].m_Format == xiiGALResourceFormat::RGBA32Float);
  const auto                                 resource = xiiLoadPipelineLibraryResource("ReflectionComposition");
  xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  XII_TEST_INT(loaded->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 2);
  XII_TEST_INT(loaded->GetPipeline()->GetExecutionOrder().GetCount(), 3);
  XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({loaded->GetAuthoring()->m_Descriptor.m_Id, resource.GetResourceID()});
  root.AddSubgraph("First", loaded->GetAuthoring()->m_Descriptor.m_Id);
  root.AddSubgraph("Second", loaded->GetAuthoring()->m_Descriptor.m_Id);
  for (const auto& port : loaded->GetPipeline()->GetInterface())
    if (port.m_Pin.m_bInput)
      for (const auto& node : root.m_Passes)
        root.m_Interface.PushBack({port.m_Pin.m_sName, true, node.m_Id, port.m_Pin.m_sName});
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
  if (compiled)
  {
    XII_TEST_INT(compiled->GetExecutionOrder().GetCount(), 6);
    XII_TEST_BOOL(compiled->GetProvenance()[0].m_InstancePath != compiled->GetProvenance()[3].m_InstancePath);
  }
}
