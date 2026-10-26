/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

void xiiCheckAuthoredSceneContract(const xiiRenderPipelineResource& scene)
{
  const auto pipeline = scene.GetPipeline();
  XII_TEST_INT(scene.GetAuthoring()->m_Descriptor.m_References.GetCount(), 5);
  XII_TEST_BOOL(pipeline->GetExtractionRequirements() == (xiiRenderExtractionCategory::Geometry | xiiRenderExtractionCategory::Lights | xiiRenderExtractionCategory::Atmosphere | xiiRenderExtractionCategory::Particles));
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
  for (const xiiStringView name : {"HDRSceneColor", "GradedColor", "SceneDepth", "ObjectId", "ViewOutput", "AmbientOcclusion", "ShadowAtlas", "ShadowVisibility", "ScreenReflections", "ReflectionLighting", "TransparentAccumulation", "TransparentRevealage", "TransparentAdditive", "TransparentModulation", "TransparentHistoryTrust"})
    XII_TEST_BOOL(pipeline->FindInterface(name, false) != xiiInvalidIndex);
  for (const xiiStringView name : {"ScreenReflections", "ReflectionLighting", "TransparentAccumulation", "TransparentRevealage", "TransparentAdditive", "TransparentModulation", "TransparentHistoryTrust"})
  {
    const auto index = pipeline->FindInterface(name, false);
    if (XII_TEST_BOOL(index != xiiInvalidIndex))
      XII_TEST_BOOL(pipeline->GetInterface()[index].m_Pin.m_bOptional);
  }
  const auto hdr = pipeline->FindInterface("HDRSceneColor", false);
  if (XII_TEST_BOOL(hdr != xiiInvalidIndex))
  {
    const auto& pin = pipeline->GetInterface()[hdr].m_Pin;
    XII_TEST_BOOL(pin.m_Format == xiiGALResourceFormat::RGBA32Float);
    XII_TEST_BOOL(pin.m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::UnorderedAccess));
  }
  const auto depth = pipeline->FindInterface("SceneDepth", false);
  if (XII_TEST_BOOL(depth != xiiInvalidIndex))
    XII_TEST_BOOL(!pipeline->GetInterface()[depth].m_Pin.m_bOptional);
  const auto& descriptor = pipeline->GetDescriptor();
  const auto  ao         = xiiFindPipelineTestPass(descriptor, "xiiAmbientOcclusionPass");
  const auto  filter     = xiiFindPipelineTestPass(descriptor, "xiiAmbientOcclusionTemporalPass");
  for (const auto& pass : descriptor.m_Passes)
  {
    if (pass.m_Id == ao)
      XII_TEST_BOOL(pass.m_Properties.GetValue("UseRayTracing") && !pass.m_Properties.GetValue("UseRayTracing")->Get<bool>());
    if (pass.m_Id == filter)
      XII_TEST_BOOL(pass.m_Properties.GetValue("UseHistory") && !pass.m_Properties.GetValue("UseHistory")->Get<bool>());
  }
}
