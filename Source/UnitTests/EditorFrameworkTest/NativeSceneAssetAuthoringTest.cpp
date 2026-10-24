/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <EditorFrameworkTest/PipelineAuthoringTestUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Types/ScopeExit.h>
namespace
{
  xiiStatus AuthorWorldDeferredLighting(xiiStringView directory)
  {
    xiiPipelineTestAuthor author(directory, "WorldDeferredLighting");
    XII_SUCCEED_OR_RETURN(author.Init(xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::BindlessResources));
    const auto* geometry    = author.Reference("World GBuffer", xiiCreateGBufferTestResource(), {0, 0});
    const auto* direct      = author.Reference("World Analytical Lighting", xiiCreateWorldSurfaceLightingTestResource(), {800, 0});
    const auto* camera      = author.Add("Camera Snapshot", "xiiViewConstantsPass", {0, 1500});
    const auto* ambient     = author.Add("Ambient Diffuse", "xiiDiffuseSurfaceLightingPass", {800, 1200});
    const auto* directSum   = author.Add("Add Directional And Local", "xiiRadianceAddPass", {1400, 0});
    const auto* ambientSum  = author.Add("Add Ambient", "xiiRadianceAddPass", {1800, 0});
    const auto* emissionSum = author.Add("Add Surface Emission", "xiiRadianceAddPass", {2200, 0});
    XII_SUCCEED_OR_RETURN(author.Connect(geometry, "SceneDepth", direct, "Depth"));
    XII_SUCCEED_OR_RETURN(author.Connect(geometry, "SceneDepth", ambient, "Depth"));
    for (const xiiStringView pin : {"Albedo", "Material", "Emissive"})
    {
      XII_SUCCEED_OR_RETURN(author.Connect(geometry, pin, direct, pin));
      XII_SUCCEED_OR_RETURN(author.Connect(geometry, pin, ambient, pin));
    }
    XII_SUCCEED_OR_RETURN(author.Connect(geometry, "Normal", direct, "Normal"));
    XII_SUCCEED_OR_RETURN(author.Connect(camera, "PerFrameCamera", direct, "Camera"));
    XII_SUCCEED_OR_RETURN(author.Connect(direct, "EnvironmentLight", ambient, "EnvironmentLight"));
    XII_SUCCEED_OR_RETURN(author.Connect(direct, "DirectionalLighting", directSum, "A"));
    XII_SUCCEED_OR_RETURN(author.Connect(direct, "LocalLighting", directSum, "B"));
    XII_SUCCEED_OR_RETURN(author.Connect(directSum, "Radiance", ambientSum, "A"));
    XII_SUCCEED_OR_RETURN(author.Connect(ambient, "Lighting", ambientSum, "B"));
    XII_SUCCEED_OR_RETURN(author.Connect(ambientSum, "Radiance", emissionSum, "A"));
    XII_SUCCEED_OR_RETURN(author.Connect(geometry, "Emissive", emissionSum, "B"));
    XII_SUCCEED_OR_RETURN(author.Boundary("InputCandidates", true, geometry, "InputCandidates", xiiRenderPipelinePin::Buffer));
    XII_SUCCEED_OR_RETURN(author.Boundary("HDRSceneColor", false, emissionSum, "Radiance"));
    XII_SUCCEED_OR_RETURN(author.Boundary("DirectLighting", false, directSum, "Radiance"));
    XII_SUCCEED_OR_RETURN(author.Boundary("AmbientLighting", false, ambient, "Lighting"));
    for (const xiiStringView pin : {"SceneDepth", "ObjectId", "Albedo", "Normal", "Material", "Emissive", "Velocity"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, false, geometry, pin));
    XII_SUCCEED_OR_RETURN(author.Boundary("LightListOverflow", false, direct, "Overflow", xiiRenderPipelinePin::Buffer));
    XII_SUCCEED_OR_RETURN(author.Boundary("VisibleMeshletOverflow", false, geometry, "VisibleMeshletOverflow", xiiRenderPipelinePin::Buffer));
    return author.Save(directory);
  }
  xiiStatus AuthorHdrDisplay(xiiStringView directory)
  {
    xiiPipelineTestAuthor author(directory, "HdrDisplay");
    XII_SUCCEED_OR_RETURN(author.Init());
    XII_SUCCEED_OR_RETURN(author.Settings("OutputSamples", 1U));
    const auto* histogram = author.Add("Luminance Histogram", "xiiLuminanceHistogramPass", {0, 0});
    const auto* exposure  = author.Add("Exposure Adaptation", "xiiAutoExposurePass", {600, 0});
    const auto* bloom     = author.Add("Bloom Prefilter", "xiiBloomPass", {600, 700});
    const auto* tone      = author.Add("Tone Mapping", "xiiToneMappingPass", {1200, 0});
    const auto* grade     = author.Add("Color Grading", "xiiColorGradingPass", {1700, 0});
    const auto* present   = author.Add("Present To View", "xiiPresentPass", {2200, 0});
    for (const auto* node : {histogram, exposure, bloom, tone, grade, present})
      XII_SUCCEED_OR_RETURN(author.Set(node, "UseViewDefaults", true));
    XII_SUCCEED_OR_RETURN(author.Boundary("HDRSceneColor", true, histogram, "HDRSceneColor"));
    XII_SUCCEED_OR_RETURN(author.Boundary("HDRSceneColor", true, bloom, "UpscaledColor"));
    XII_SUCCEED_OR_RETURN(author.Boundary("HDRSceneColor", true, tone, "UpscaledColor"));
    XII_SUCCEED_OR_RETURN(author.Connect(histogram, "LuminanceHistogram", exposure, "LuminanceHistogram"));
    XII_SUCCEED_OR_RETURN(author.Connect(exposure, "CurrentExposure", tone, "CurrentExposure"));
    XII_SUCCEED_OR_RETURN(author.Connect(bloom, "BloomTexture", tone, "BloomTexture"));
    XII_SUCCEED_OR_RETURN(author.Connect(tone, "DisplayLinearColor", grade, "DisplayLinearColor"));
    XII_SUCCEED_OR_RETURN(author.Connect(grade, "GradedColor", present, "GradedColor"));
    XII_SUCCEED_OR_RETURN(author.Boundary("ViewOutput", false, present, "ViewOutput"));
    XII_SUCCEED_OR_RETURN(author.Boundary("CurrentExposure", false, exposure, "CurrentExposure", xiiRenderPipelinePin::Buffer));
    XII_SUCCEED_OR_RETURN(author.Boundary("DisplayLinearColor", false, tone, "DisplayLinearColor"));
    XII_SUCCEED_OR_RETURN(author.Boundary("GradedColor", false, grade, "GradedColor"));
    return author.Save(directory);
  }
  xiiStatus AuthorDeferredOpaque(xiiStringView directory)
  {
    xiiPipelineTestAuthor author(directory, "DeferredOpaque");
    XII_SUCCEED_OR_RETURN(author.Init(xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::BindlessResources));
    XII_SUCCEED_OR_RETURN(author.Settings("OutputSamples", 1U));
    const auto* world   = author.Reference("Opaque World Lighting", xiiCreateWorldDeferredLightingTestResource(), {0, 0});
    const auto* display = author.Reference("Display Output", xiiCreateHdrDisplayTestResource(), {1200, 0});
    XII_SUCCEED_OR_RETURN(author.Connect(world, "HDRSceneColor", display, "HDRSceneColor"));
    XII_SUCCEED_OR_RETURN(author.Boundary("InputCandidates", true, world, "InputCandidates", xiiRenderPipelinePin::Buffer));
    for (const xiiStringView pin : {"HDRSceneColor", "DirectLighting", "AmbientLighting", "SceneDepth", "ObjectId", "Albedo", "Normal", "Material", "Emissive", "Velocity"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, false, world, pin));
    for (const xiiStringView pin : {"LightListOverflow", "VisibleMeshletOverflow"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, false, world, pin, xiiRenderPipelinePin::Buffer));
    XII_SUCCEED_OR_RETURN(author.Boundary("ViewOutput", false, display, "ViewOutput"));
    XII_SUCCEED_OR_RETURN(author.Boundary("CurrentExposure", false, display, "CurrentExposure", xiiRenderPipelinePin::Buffer));
    return author.Save(directory);
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeSceneAuthoring);
XII_CREATE_SIMPLE_TEST(NativeSceneAuthoring, CompleteOpaqueSceneAssets)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "NativeSceneAuthoring", "SceneBase", xiiDataDirUsage::ReadOnly)) || !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeSceneAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeSceneAuthoring"));
  XII_TEST_STATUS(AuthorWorldDeferredLighting(path));
  XII_TEST_STATUS(AuthorHdrDisplay(path));
  XII_TEST_STATUS(AuthorDeferredOpaque(path));
}
