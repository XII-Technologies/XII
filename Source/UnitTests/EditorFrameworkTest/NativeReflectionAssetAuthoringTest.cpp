/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <EditorFrameworkTest/PipelineAuthoringTestUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Types/ScopeExit.h>
namespace
{
  xiiStatus AuthorReflectionLeaf(xiiStringView path, xiiStringView name, xiiStringView type, xiiArrayPtr<const xiiStringView> inputs)
  {
    xiiPipelineTestAuthor author(path, name);
    XII_SUCCEED_OR_RETURN(author.Init());
    const auto* operation = author.Add("Operation", type, {0, 0});
    for (const auto input : inputs)
      XII_SUCCEED_OR_RETURN(author.Boundary(input, true, operation, input));
    XII_SUCCEED_OR_RETURN(author.Boundary("Radiance", false, operation, "Radiance"));
    return author.Save(path);
  }
  xiiStatus AuthorReflectionSelection(xiiStringView path)
  {
    xiiPipelineTestAuthor author(path, "ReflectionSelection");
    XII_SUCCEED_OR_RETURN(author.Init());
    const auto* traced = author.Add("Traced Detail Over Environment", "xiiConfidenceBlendPass", {0, 0});
    const auto* screen = author.Add("Screen Detail Over Traced Result", "xiiConfidenceBlendPass", {700, 0});
    XII_SUCCEED_OR_RETURN(author.Boundary("EnvironmentSpecular", true, traced, "Background"));
    XII_SUCCEED_OR_RETURN(author.Boundary("RTFinalReflections", true, traced, "Layer"));
    XII_SUCCEED_OR_RETURN(author.Connect(traced, "Radiance", screen, "Background"));
    XII_SUCCEED_OR_RETURN(author.Boundary("SSRTerm", true, screen, "Layer"));
    XII_SUCCEED_OR_RETURN(author.Boundary("SelectedReflection", false, screen, "Radiance"));
    return author.Save(path);
  }
  xiiStatus AuthorReflectionComposition(xiiStringView path)
  {
    xiiPipelineTestAuthor author(path, "ReflectionComposition");
    XII_SUCCEED_OR_RETURN(author.Init());
    const auto* select  = author.Reference("Reflection Priority", xiiCreateReflectionSelectionTestResource(), {0, 0});
    const auto* replace = author.Add("Replace Environment Contribution", "xiiRadianceReplacePass", {900, 0});
    for (const xiiStringView input : {"EnvironmentSpecular", "RTFinalReflections", "SSRTerm"})
      XII_SUCCEED_OR_RETURN(author.Boundary(input, true, select, input));
    XII_SUCCEED_OR_RETURN(author.Boundary("HDRSceneColor", true, replace, "Scene"));
    XII_SUCCEED_OR_RETURN(author.Boundary("EnvironmentSpecular", true, replace, "PreviousContribution"));
    XII_SUCCEED_OR_RETURN(author.Connect(select, "SelectedReflection", replace, "Replacement"));
    XII_SUCCEED_OR_RETURN(author.Boundary("SelectedReflection", false, select, "SelectedReflection"));
    XII_SUCCEED_OR_RETURN(author.Boundary("HDRSceneColor", false, replace, "Radiance"));
    return author.Save(path);
  }
  xiiStatus AuthorOpaqueComposition(xiiStringView path)
  {
    xiiPipelineTestAuthor author(path, "OpaqueComposition");
    XII_SUCCEED_OR_RETURN(author.Init());
    const auto* opaque    = author.Reference("Opaque Surface Radiance", xiiCreateSurfaceRadianceCompositionTestResource(), {0, 0});
    const auto* scene     = author.Add("Select Scene Radiance", "xiiTextureSelectionPass", {700, 0});
    const auto* trace     = author.Add("Trace Screen Reflections", "xiiScreenSpaceReflectionsPass", {1400, 0});
    const auto* raw       = author.Add("Select Raw Reflections", "xiiTextureSelectionPass", {2100, 0});
    const auto* temporal  = author.Add("Resolve Temporal Reflections", "xiiScreenSpaceReflectionsTemporalPass", {2800, 0});
    const auto* resolved  = author.Add("Select Resolved Reflections", "xiiTextureSelectionPass", {3500, 0});
    const auto* composite = author.Reference("Compose Reflection Contributions", xiiCreateReflectionCompositionTestResource(), {4200, 0});
    for (const xiiStringView pin : {"DirectLighting", "IndirectLighting", "GBufferEmissive", "RTFinalGI", "VolumetricScattering"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, opaque, pin));
    XII_SUCCEED_OR_RETURN(author.Boundary("HDRSceneColor", true, scene, "Primary"));
    XII_SUCCEED_OR_RETURN(author.Connect(opaque, "HDRSceneColor", scene, "Fallback"));
    for (const xiiStringView pin : {"SceneDepth", "GBufferNormal", "GBufferMaterial"})
    {
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, trace, pin));
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, temporal, pin));
    }
    for (const xiiStringView pin : {"GBufferAlbedo", "BRDFLut", "HiZClosestPyramid", "HiZPyramid"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, trace, pin));
    for (const xiiStringView pin : {"GBufferObjectId", "DilatedVelocityBuffer", "ReSTIRDISurfacePrevious"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, temporal, pin));
    XII_SUCCEED_OR_RETURN(author.Boundary("PerFrameCamera", true, trace, "PerFrameCamera", xiiRenderPipelinePin::Buffer));
    XII_SUCCEED_OR_RETURN(author.Boundary("PerFrameCamera", true, temporal, "PerFrameCamera", xiiRenderPipelinePin::Buffer));
    XII_SUCCEED_OR_RETURN(author.Boundary("PerFrameGlobal", true, trace, "PerFrameGlobal", xiiRenderPipelinePin::Buffer));
    XII_SUCCEED_OR_RETURN(author.Connect(scene, "Texture", trace, "HDRSceneColor"));
    XII_SUCCEED_OR_RETURN(author.Connect(scene, "Texture", composite, "HDRSceneColor"));
    XII_SUCCEED_OR_RETURN(author.Connect(trace, "SSRRawTerm", raw, "Fallback"));
    XII_SUCCEED_OR_RETURN(author.Boundary("SSRRawTerm", true, raw, "Primary"));
    XII_SUCCEED_OR_RETURN(author.Connect(raw, "Texture", temporal, "SSRRawTerm"));
    XII_SUCCEED_OR_RETURN(author.Connect(temporal, "SSRTerm", resolved, "Fallback"));
    XII_SUCCEED_OR_RETURN(author.Boundary("SSRTerm", true, resolved, "Primary"));
    XII_SUCCEED_OR_RETURN(author.Connect(resolved, "Texture", composite, "SSRTerm"));
    XII_SUCCEED_OR_RETURN(author.Boundary("EnvironmentSpecular", true, composite, "EnvironmentSpecular"));
    XII_SUCCEED_OR_RETURN(author.Boundary("RTFinalReflections", true, composite, "RTFinalReflections"));
    XII_SUCCEED_OR_RETURN(author.Boundary("HDRSceneColor", false, composite, "HDRSceneColor"));
    XII_SUCCEED_OR_RETURN(author.Boundary("SelectedReflection", false, composite, "SelectedReflection"));
    XII_SUCCEED_OR_RETURN(author.Boundary("SSRRawTerm", false, raw, "Texture"));
    XII_SUCCEED_OR_RETURN(author.Boundary("SSRTerm", false, resolved, "Texture"));
    XII_SUCCEED_OR_RETURN(author.Boundary("PreviousReflection", false, temporal, "PreviousReflection"));
    XII_SUCCEED_OR_RETURN(author.Boundary("xiiSSRConstants", false, trace, "xiiSSRConstants", xiiRenderPipelinePin::Buffer));
    XII_SUCCEED_OR_RETURN(author.Boundary("xiiSSRTemporalConstants", false, temporal, "xiiSSRTemporalConstants", xiiRenderPipelinePin::Buffer));
    return author.Save(path);
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeReflectionAuthoring);
XII_CREATE_SIMPLE_TEST(NativeReflectionAuthoring, ConfidenceLayersAndOpaqueComposition)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "NativeReflectionAuthoring", "ReflectionBase", xiiDataDirUsage::ReadOnly)) || !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeReflectionAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeReflectionAuthoring"));
  const xiiStringView blend[] = {"Background", "Layer"}, replace[] = {"Scene", "PreviousContribution", "Replacement"};
  XII_TEST_STATUS(AuthorReflectionLeaf(path, "ConfidenceBlend", "xiiConfidenceBlendPass", blend));
  XII_TEST_STATUS(AuthorReflectionLeaf(path, "RadianceReplace", "xiiRadianceReplacePass", replace));
  XII_TEST_STATUS(AuthorReflectionSelection(path));
  XII_TEST_STATUS(AuthorReflectionComposition(path));
  XII_TEST_STATUS(AuthorOpaqueComposition(path));
}
