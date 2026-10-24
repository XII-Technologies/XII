/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

void xiiCheckAuthoredSceneContract(const xiiRenderPipelineResource& scene)
{
  const auto pipeline = scene.GetPipeline();
  XII_TEST_BOOL(scene.GetAuthoring()->m_Descriptor.m_References.IsEmpty());
  XII_TEST_BOOL(pipeline->GetExtractionRequirements() == (xiiRenderExtractionCategory::Geometry | xiiRenderExtractionCategory::Lights));
  XII_TEST_BOOL(pipeline->GetRuntimeFeatures() == (xiiRenderPipelineFeature::GpuScene | xiiRenderPipelineFeature::Lighting));
  XII_TEST_INT(pipeline->GetDescriptor().m_Settings.m_uiOutputSamples, 1);
  for (const auto& pass : pipeline->GetDescriptor().m_Passes)
    XII_TEST_BOOL(pass.m_pType != xiiGetStaticRTTI<xiiRenderPipelineSubgraphPass>());
  for (const xiiStringView name : {"ViewTarget", "InputCandidates"})
  {
    const auto index = pipeline->FindInterface(name, true);
    if (XII_TEST_BOOL(index != xiiInvalidIndex))
      XII_TEST_BOOL(pipeline->GetInterface()[index].m_Pin.m_bOptional);
  }
  for (const xiiStringView name : {"HDRSceneColor", "GradedColor", "SceneDepth", "ObjectId", "ViewOutput"})
    XII_TEST_BOOL(pipeline->FindInterface(name, false) != xiiInvalidIndex);
}
