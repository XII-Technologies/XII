/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
XII_CREATE_SIMPLE_TEST(Pipeline, AuthoredOpaqueSceneResources)
{
  const auto                                 worldAsset = xiiLoadPipelineLibraryResource("WorldDeferredLighting"), displayAsset = xiiLoadPipelineLibraryResource("HdrDisplay"), sceneAsset = xiiLoadPipelineLibraryResource("DeferredOpaque");
  xiiResourceLock<xiiRenderPipelineResource> world(worldAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail), display(displayAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail), scene(sceneAsset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(world.IsValid() && display.IsValid() && scene.IsValid() && world->GetPipeline() && display->GetPipeline() && scene->GetPipeline()))
    return;
  XII_TEST_INT(world->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 7);
  XII_TEST_INT(display->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 6);
  XII_TEST_INT(scene->GetAuthoring()->m_Descriptor.m_Passes.GetCount(), 2);
  XII_TEST_BOOL(world->GetPipeline()->GetExtractionRequirements() == (xiiRenderExtractionCategory::Geometry | xiiRenderExtractionCategory::Lights));
  XII_TEST_BOOL(world->GetPipeline()->GetRuntimeFeatures() == (xiiRenderPipelineFeature::GpuScene | xiiRenderPipelineFeature::Lighting));
  XII_TEST_BOOL(display->GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && display->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
  XII_TEST_INT(scene->GetPipeline()->GetExecutionOrder().GetCount(), world->GetPipeline()->GetExecutionOrder().GetCount() + 6);
  XII_TEST_INT(scene->GetAuthoring()->m_Descriptor.m_Settings.m_uiOutputSamples, 1);
  xiiUInt32 inputs = 0;
  for (const auto& port : scene->GetPipeline()->GetInterface())
    if (port.m_Pin.m_bInput)
    {
      ++inputs;
      XII_TEST_BOOL(port.m_Pin.m_sName == "InputCandidates" && port.m_Pin.m_bOptional);
    }
  XII_TEST_INT(inputs, 1);
}
