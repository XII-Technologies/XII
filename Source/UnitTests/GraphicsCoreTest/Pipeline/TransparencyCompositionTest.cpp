/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/IO/MemoryStream.h>
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

  const auto                                 resource = xiiLoadPipelineLibraryResource("TransparencyResolve");
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
