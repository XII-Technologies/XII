/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Material/MaterialGpuStorage.h>
#include <GraphicsCore/Pipeline/Passes/Particles/ParticleIntegratePass.h>
#include <GraphicsCore/Pipeline/Passes/Particles/ParticleIntegrationConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Transparency/WeightedBlendedOITResolvePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

XII_CREATE_SIMPLE_TEST(Pipeline, AuthoredTransparencyResolve)
{
  xiiParticleIntegrationConstantsPass constants;
  constants.m_uiActiveCount = 123;
  constants.m_WindVelocity  = xiiVec3(1, 2, 3);
  xiiVariantDictionary properties;
  XII_TEST_RESULT(constants.GetProperties(properties));
  xiiParticleIntegrationConstantsPass copy;
  XII_TEST_RESULT(copy.Configure(properties));
  XII_TEST_INT(copy.m_uiActiveCount, 123);
  XII_TEST_BOOL(copy.m_WindVelocity == xiiVec3(1, 2, 3));
  properties.Insert("DeltaTimeS", 1.0f);
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  properties.Clear();
  properties.Insert("DragCoefficient", -1.0f);
  XII_TEST_BOOL(copy.Configure(properties).Failed());
  xiiParticleIntegratePass integrate;
  properties.Clear();
  properties.Insert("ParticleCount", xiiUInt32(-1));
  XII_TEST_BOOL(integrate.Configure(properties).Failed());
  xiiWeightedBlendedOITResolvePass resolve;
  resolve.m_bBypass = true;
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  resolve.GetPins(pins);
  for (const auto& pin : pins)
    if (pin.m_sName == "OITAccumulate" || pin.m_sName == "OITReveal")
      XII_TEST_BOOL(pin.m_bOptional);
  XII_TEST_BOOL(resolve.GetRequiredDeviceFeatures().IsNoFlagSet());

  const auto                                 resource = xiiCreateTransparencyResolveTestResource();
  xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  const auto descriptor = loaded->GetAuthoring()->m_Descriptor;
  XII_TEST_INT(descriptor.m_Passes.GetCount(), 8);
  XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet());
  XII_TEST_BOOL(loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
  xiiGraphDefinition definition;
  XII_TEST_RESULT(descriptor.MakeGraphDefinition(definition));
  xiiDynamicArray<xiiRenderPipelinePin> interfacePins;
  XII_TEST_RESULT(xiiGetRenderPipelineInterface(descriptor, interfacePins));
  xiiRenderPipelineResourceDescriptor root;
  root.m_Definitions.PushBack(definition);
  root.AddSubgraph("First", definition.m_Id);
  root.AddSubgraph("Second", definition.m_Id);
  for (const auto& pin : interfacePins)
    if (pin.m_bInput)
      for (const auto& pass : root.m_Passes)
        root.m_Interface.PushBack({pin.m_sName, true, pass.m_Id, pin.m_sName});
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
  if (compiled)
    XII_TEST_INT(compiled->GetExecutionOrder().GetCount(), 16);
}

XII_CREATE_SIMPLE_TEST(Pipeline, ImmutableGpuSurfaceMaterialSnapshot)
{
  xiiMaterialSchemaDescription description;
  description.m_sName = "Snapshot-owned canonical material";
  description.AddParameter("BaseColor", xiiMaterialParameterType::Color, xiiColor(0.2f, 0.4f, 0.8f, 0.5f));
  description.AddParameter("EmissiveColor", xiiMaterialParameterType::Float3, xiiVec3(0.1f, 0.2f, 0.3f));
  description.AddParameter("Roughness", xiiMaterialParameterType::Float, 0.25f);
  description.AddTexture("BaseColorTexture");
  xiiSharedPtr<xiiMaterialSchema> schema = XII_DEFAULT_NEW(xiiMaterialSchema);
  if (!XII_TEST_RESULT(schema->Build(description)))
    return;
  xiiMaterialRuntimeState runtime;
  runtime.m_BlendMode = xiiMaterialBlendMode::Translucent;
  xiiMaterialInstance material;
  if (!XII_TEST_RESULT(material.Initialize(schema, runtime)))
    return;
  const auto texture = xiiMaterialParameterId::Make("BaseColorTexture");
  XII_TEST_RESULT(material.SetBindlessIndex(texture, 7, 3));
  xiiMaterialInstanceSnapshot snapshot;
  material.CreateSnapshot(snapshot);
  XII_TEST_RESULT(material.SetParameter("BaseColor", xiiColor(1, 0, 0, 0.1f)));
  XII_TEST_RESULT(material.SetParameter("Roughness", 0.8f));
  XII_TEST_RESULT(material.SetBindlessIndex(texture, 23, 5));
  runtime.m_BlendMode = xiiMaterialBlendMode::Opaque;
  material.SetRuntimeState(runtime);
  const auto first = xiiResolveGpuSurfaceMaterial(snapshot);
  XII_TEST_BOOL(first.BaseColorOpacity == xiiVec4(0.2f, 0.4f, 0.8f, 0.5f));
  XII_TEST_BOOL(first.EmissiveColorAndRoughness == xiiVec4(0.1f, 0.2f, 0.3f, 0.25f));
  XII_TEST_INT(first.TextureIndices0.x, 7);
  XII_TEST_INT(first.SamplerIndices0.x, 3);
  XII_TEST_INT(first.Metadata.z, xiiMaterialAlphaMode::Blend);
  material.CreateSnapshot(snapshot);
  const auto second = xiiResolveGpuSurfaceMaterial(snapshot);
  XII_TEST_BOOL(second.BaseColorOpacity == xiiVec4(1, 0, 0, 0.1f));
  XII_TEST_FLOAT(second.EmissiveColorAndRoughness.w, 0.8f, 0);
  XII_TEST_INT(second.TextureIndices0.x, 23);
  XII_TEST_INT(second.SamplerIndices0.x, 5);
  XII_TEST_INT(second.Metadata.z, xiiMaterialAlphaMode::Opaque);
}
