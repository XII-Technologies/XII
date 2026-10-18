/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/IrradianceLayerPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/DiffuseSurfaceLightingPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/EnvironmentSpecularPass.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
XII_CREATE_SIMPLE_TEST(Pipeline, IndependentIndirectSurfaceSchemas)
{
  xiiDiffuseSurfaceLightingPass diffuse;
  xiiVariantDictionary          properties;
  properties.Insert("Scale", 2.0f);
  XII_TEST_RESULT(diffuse.Configure(properties));
  properties.Clear();
  XII_TEST_RESULT(diffuse.GetProperties(properties));
  XII_TEST_BOOL(!properties.Contains("Depth") && !properties.Contains("EnvironmentLight"));
  xiiDiffuseSurfaceLightingPass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_FLOAT(copy.m_fScale, 2, 0);
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  diffuse.GetPins(pins);
  XII_TEST_RESULT(diffuse.InitializePinBindings(pins));
  for (const auto& pin : pins)
    XII_TEST_BOOL(pin.m_sName != "Camera" && pin.m_sName != "Normal");
  XII_TEST_BOOL(pins[diffuse.m_Irradiance.GetBindingIndex()].m_bOptional && pins[diffuse.m_Occlusion.GetBindingIndex()].m_bOptional);
  XII_TEST_BOOL(pins[diffuse.m_Environment.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::UniformBuffer));
  xiiEnvironmentSpecularPass specular;
  pins.Clear();
  specular.GetPins(pins);
  XII_TEST_RESULT(specular.InitializePinBindings(pins));
  for (const auto& pin : pins)
    XII_TEST_BOOL(pin.m_sName != "Emissive" && pin.m_sName != "EnvironmentLight");
  XII_TEST_BOOL(!pins[specular.m_Camera.GetBindingIndex()].m_bOptional && pins[specular.m_Output.GetBindingIndex()].m_Format == xiiGALResourceFormat::RGBA32Float);
  xiiIrradianceLayerPass layer;
  pins.Clear();
  layer.GetPins(pins);
  XII_TEST_RESULT(layer.InitializePinBindings(pins));
  XII_TEST_BOOL(!pins[layer.m_Detail.GetBindingIndex()].m_bOptional);
  XII_TEST_BOOL(diffuse.GetExtractionRequirements().IsNoFlagSet() && specular.GetExtractionRequirements().IsNoFlagSet() && layer.GetExtractionRequirements().IsNoFlagSet());
  XII_TEST_BOOL(diffuse.GetRuntimeFeatures().IsNoFlagSet() && specular.GetRuntimeFeatures().IsNoFlagSet() && layer.GetRuntimeFeatures().IsNoFlagSet());
  properties.Clear();
  properties.Insert("Scale", -1.0f);
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  XII_TEST_BOOL(specular.Configure(properties).Failed());
}
XII_CREATE_SIMPLE_TEST(Pipeline, AuthoredIndirectSurfaceComposition)
{
  const auto                                 asset = xiiLoadPipelineLibraryResource("SurfaceIndirectLighting");
  xiiResourceLock<xiiRenderPipelineResource> source(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(source.IsValid() && source->GetAuthoring() && source->GetPipeline()))
    return;
  const auto authoring = source->GetAuthoring();
  XII_TEST_INT(authoring->m_Descriptor.m_Passes.GetCount(), 3);
  XII_TEST_INT(source->GetPipeline()->GetExecutionOrder().GetCount(), 4);
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({authoring->m_Descriptor.m_Id, asset.GetResourceID()});
  const auto a = root.AddSubgraph("First indirect", authoring->m_Descriptor.m_Id), b = root.AddSubgraph("Second indirect", authoring->m_Descriptor.m_Id);
  for (const auto& port : source->GetPipeline()->GetInterface())
    if (port.m_Pin.m_bInput)
    {
      root.m_Interface.PushBack({port.m_Pin.m_sName, true, root.m_Passes[a].m_Id, port.m_Pin.m_sName});
      root.m_Interface.PushBack({port.m_Pin.m_sName, true, root.m_Passes[b].m_Id, port.m_Pin.m_sName});
    }
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
  if (compiled)
  {
    XII_TEST_INT(compiled->GetExecutionOrder().GetCount(), 8);
    XII_TEST_BOOL(compiled->GetProvenance()[0].m_InstancePath != compiled->GetProvenance()[4].m_InstancePath);
    XII_TEST_BOOL(compiled->GetRuntimeFeatures().IsNoFlagSet() && compiled->GetExtractionRequirements().IsNoFlagSet());
  }
}
