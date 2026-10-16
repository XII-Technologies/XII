/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <GraphicsCore/Pipeline/Passes/Particles/ParticleIntegratePass.h>
#include <GraphicsCore/Pipeline/Passes/Particles/ParticleIntegrationConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Transparency/WeightedBlendedOITResolvePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

XII_CREATE_SIMPLE_TEST(Pipeline, AuthoredTransparencyResolve)
{
  xiiRenderPipelineResourceDescriptor legacy;
  xiiVariantDictionary                properties;
  properties.Insert("Stage", "Transparency");
  legacy.AddPass("Transparency", "xiiBuiltinRenderPipelinePass", properties);
  const auto nodeId = legacy.m_Passes[0].m_Id;
  legacy.m_Passes[0].m_Parameters.Insert("Local", 7U);
  xiiGraphDefinition nested;
  XII_TEST_RESULT(legacy.MakeGraphDefinition(nested));
  legacy.m_Definitions.PushBack(nested);
  xiiContiguousMemoryStreamStorage bytes;
  xiiMemoryStreamWriter            writer(&bytes);
  XII_TEST_RESULT(legacy.Serialize(writer));
  xiiMemoryStreamReader               reader(&bytes);
  xiiRenderPipelineResourceDescriptor migrated;
  XII_TEST_RESULT(migrated.Deserialize(reader));
  XII_TEST_BOOL(migrated.m_Passes[0].m_Id == nodeId && migrated.m_Passes[0].m_sType.IsEmpty());
  XII_TEST_BOOL(migrated.m_Passes[0].m_Definition.IsValid());
  XII_TEST_BOOL(migrated.m_Passes[0].m_Parameters == legacy.m_Passes[0].m_Parameters);
  XII_TEST_BOOL(migrated.m_Definitions[0].m_Nodes[0].m_sType.IsEmpty());
  XII_TEST_BOOL(migrated.m_Definitions[0].m_Nodes[0].m_Definition == migrated.m_Passes[0].m_Definition);
  XII_TEST_INT(migrated.m_References.GetCount(), 1);
  XII_TEST_BOOL(migrated.m_References[0].m_sResource == "RenderPipelines/TransparencyResolve.xiiBinRenderPipeline");
  xiiContiguousMemoryStreamStorage first, second;
  xiiMemoryStreamWriter            firstWriter(&first), secondWriter(&second);
  XII_TEST_RESULT(migrated.Serialize(firstWriter));
  xiiMemoryStreamReader               firstReader(&first);
  xiiRenderPipelineResourceDescriptor repeated;
  XII_TEST_RESULT(repeated.Deserialize(firstReader));
  XII_TEST_RESULT(repeated.Serialize(secondWriter));
  XII_TEST_BOOL(xiiMakeArrayPtr(first.GetData(), first.GetStorageSize32()) == xiiMakeArrayPtr(second.GetData(), second.GetStorageSize32()));

  xiiAbstractObjectGraph graph, types;
  auto*                  old = graph.AddNode(nodeId, "xiiBuiltinRenderPipelinePass", 6);
  old->AddProperty("Stage", "Transparency");
  auto* type = types.AddNode(xiiUuid::MakeUuid(), "xiiReflectedTypeDescriptor", 1);
  type->AddProperty("TypeName", "xiiBuiltinRenderPipelinePass");
  type->AddProperty("ParentTypeName", "xiiRenderPipelinePass");
  type->AddProperty("TypeVersion", 6U);
  xiiGraphVersioning::GetSingleton()->PatchGraph(&graph, &types);
  XII_TEST_BOOL(old->GetGuid() == nodeId && old->GetType() == "xiiRenderPipelineSubgraphPass");
  XII_TEST_BOOL(old->FindProperty("Resource") && !old->FindProperty("Stage"));

  xiiParticleIntegrationConstantsPass constants;
  constants.m_uiActiveCount = 123;
  constants.m_WindVelocity  = xiiVec3(1, 2, 3);
  properties.Clear();
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

  const auto                                 resource = xiiLoadPipelineLibraryResource("TransparencyResolve");
  xiiResourceLock<xiiRenderPipelineResource> loaded(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  const auto descriptor = loaded->GetAuthoring()->m_Descriptor;
  XII_TEST_INT(descriptor.m_Passes.GetCount(), 8);
  XII_TEST_BOOL(loaded->GetPipeline()->GetExtractionRequirements().IsNoFlagSet());
  XII_TEST_BOOL(loaded->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
  for (const auto& pass : descriptor.m_Passes)
    XII_TEST_BOOL(pass.m_sType != "xiiBuiltinRenderPipelinePass");
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
