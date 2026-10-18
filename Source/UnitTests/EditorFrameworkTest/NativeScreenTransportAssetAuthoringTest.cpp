/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <EditorFrameworkTest/PipelineAuthoringTestUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Types/ScopeExit.h>
namespace
{
  xiiStatus AuthorTransportLeaf(xiiStringView path, xiiStringView name, xiiStringView typeName)
  {
    const auto* type = xiiRTTI::FindTypeByName(typeName);
    if (!type)
      return xiiStatus("Missing transport operation type.");
    xiiRenderPipelinePass* pass = type->GetAllocator()->Allocate<xiiRenderPipelinePass>();
    XII_SCOPE_EXIT(type->GetAllocator()->Deallocate(pass));
    xiiPipelineTestAuthor author(path, name);
    XII_SUCCEED_OR_RETURN(author.Init());
    const auto*                           node = author.Add("Operation", typeName, {0, 0});
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    pass->GetPins(pins);
    for (const auto& pin : pins)
      XII_SUCCEED_OR_RETURN(author.Boundary(pin.m_sName, pin.m_bInput, node, pin.m_sName, pin.m_Type));
    return author.Save(path);
  }
  xiiStatus AuthorScreenDiffuseLighting(xiiStringView path)
  {
    xiiPipelineTestAuthor author(path, "ScreenDiffuseLighting");
    XII_SUCCEED_OR_RETURN(author.Init());
    const auto* trace       = author.Reference("Gather Incident Irradiance", "ScreenIrradiance", {0, 0});
    const auto* environment = author.Add("No Additional Ambient Baseline", "xiiEnvironmentLightConstantsPass", {700, 1000});
    const auto* diffuse     = author.Add("Evaluate Diffuse Material", "xiiDiffuseSurfaceLightingPass", {1400, 0});
    XII_SUCCEED_OR_RETURN(author.Set(environment, "AmbientRadiance", xiiVec3::MakeZero()));
    XII_SUCCEED_OR_RETURN(author.Boundary("Before", true, trace, "Before", xiiRenderPipelinePin::Order));
    XII_SUCCEED_OR_RETURN(author.Boundary("Before", true, environment, "Before", xiiRenderPipelinePin::Order));
    XII_SUCCEED_OR_RETURN(author.Boundary("Depth", true, trace, "Depth"));
    XII_SUCCEED_OR_RETURN(author.Boundary("Depth", true, diffuse, "Depth"));
    for (const xiiStringView pin : {"Normal", "Radiance"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, trace, pin));
    XII_SUCCEED_OR_RETURN(author.Boundary("Camera", true, trace, "Camera", xiiRenderPipelinePin::Buffer));
    for (const xiiStringView pin : {"Albedo", "Material", "Emissive"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, diffuse, pin));
    XII_SUCCEED_OR_RETURN(author.Connect(trace, "Irradiance", diffuse, "Irradiance"));
    XII_SUCCEED_OR_RETURN(author.Connect(environment, "EnvironmentLight", diffuse, "EnvironmentLight"));
    XII_SUCCEED_OR_RETURN(author.Boundary("Irradiance", false, trace, "Irradiance"));
    XII_SUCCEED_OR_RETURN(author.Boundary("Lighting", false, diffuse, "Lighting"));
    XII_SUCCEED_OR_RETURN(author.Boundary("After", false, diffuse, "After", xiiRenderPipelinePin::Order));
    return author.Save(path);
  }
  xiiStatus AuthorScreenRefraction(xiiStringView path)
  {
    xiiPipelineTestAuthor author(path, "ScreenRefraction");
    XII_SUCCEED_OR_RETURN(author.Init());
    const auto* trace  = author.Reference("Sample Background Refraction", "ScreenRefractionTrace", {0, 0});
    const auto* weight = author.Add("Apply Transmission Weight", "xiiConfidenceRadiancePass", {700, 0});
    const auto* sum    = author.Add("Add Transmitted Radiance", "xiiRadianceAddPass", {1400, 0});
    XII_SUCCEED_OR_RETURN(author.Boundary("Before", true, trace, "Before", xiiRenderPipelinePin::Order));
    for (const xiiStringView pin : {"Depth", "Normal", "Material", "Emissive"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, trace, pin));
    XII_SUCCEED_OR_RETURN(author.Boundary("BackgroundRadiance", true, trace, "Radiance"));
    XII_SUCCEED_OR_RETURN(author.Boundary("Camera", true, trace, "Camera", xiiRenderPipelinePin::Buffer));
    XII_SUCCEED_OR_RETURN(author.Boundary("Radiance", true, sum, "A"));
    XII_SUCCEED_OR_RETURN(author.Connect(trace, "Refraction", weight, "Signal"));
    XII_SUCCEED_OR_RETURN(author.Connect(weight, "Radiance", sum, "B"));
    XII_SUCCEED_OR_RETURN(author.Boundary("RefractionSignal", false, trace, "Refraction"));
    XII_SUCCEED_OR_RETURN(author.Boundary("TransmittedRadiance", false, weight, "Radiance"));
    XII_SUCCEED_OR_RETURN(author.Boundary("Radiance", false, sum, "Radiance"));
    XII_SUCCEED_OR_RETURN(author.Boundary("After", false, sum, "After", xiiRenderPipelinePin::Order));
    return author.Save(path);
  }
  xiiStatus AuthorScreenEffects(xiiStringView path)
  {
    xiiPipelineTestAuthor author(path, "ScreenEffects");
    XII_SUCCEED_OR_RETURN(author.Init());
    const auto* diffuse          = author.Reference("Screen Diffuse Lighting", "ScreenDiffuseLighting", {0, 0});
    const auto* selectGI         = author.Add("Select Screen Lighting", "xiiTextureSelectionPass", {700, 0});
    const auto* sum              = author.Add("Add Screen Lighting", "xiiRadianceAddPass", {1400, 0});
    const auto* selectBackground = author.Add("Select Refraction Background", "xiiTextureSelectionPass", {2100, 0});
    const auto* refract          = author.Reference("Screen Refraction", "ScreenRefraction", {2800, 0});
    const auto* settings         = author.Add("World Atmosphere Parameters", "xiiAtmosphereConstantsPass", {2800, 1200});
    const auto* atmosphere       = author.Reference("Atmospheric View", "AtmosphereApply", {3700, 0});
    XII_SUCCEED_OR_RETURN(author.Set(settings, "UseWorldAtmosphere", true));
    XII_SUCCEED_OR_RETURN(author.Boundary("Before", true, diffuse, "Before", xiiRenderPipelinePin::Order));
    XII_SUCCEED_OR_RETURN(author.Boundary("Before", true, settings, "Before", xiiRenderPipelinePin::Order));
    for (const xiiStringView pin : {"Depth", "Normal", "Material", "Emissive"})
    {
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, diffuse, pin));
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, refract, pin));
    }
    XII_SUCCEED_OR_RETURN(author.Boundary("Albedo", true, diffuse, "Albedo"));
    XII_SUCCEED_OR_RETURN(author.Boundary("Radiance", true, diffuse, "Radiance"));
    XII_SUCCEED_OR_RETURN(author.Boundary("Radiance", true, sum, "A"));
    for (const auto* node : {diffuse, refract, atmosphere})
      XII_SUCCEED_OR_RETURN(author.Boundary("Camera", true, node, "Camera", xiiRenderPipelinePin::Buffer));
    XII_SUCCEED_OR_RETURN(author.Boundary("ScreenLighting", true, selectGI, "Primary"));
    XII_SUCCEED_OR_RETURN(author.Connect(diffuse, "Lighting", selectGI, "Fallback"));
    XII_SUCCEED_OR_RETURN(author.Connect(selectGI, "Texture", sum, "B"));
    XII_SUCCEED_OR_RETURN(author.Boundary("RefractionBackground", true, selectBackground, "Primary"));
    XII_SUCCEED_OR_RETURN(author.Connect(sum, "Radiance", selectBackground, "Fallback"));
    XII_SUCCEED_OR_RETURN(author.Connect(selectBackground, "Texture", refract, "BackgroundRadiance"));
    XII_SUCCEED_OR_RETURN(author.Connect(sum, "Radiance", refract, "Radiance"));
    XII_SUCCEED_OR_RETURN(author.Connect(refract, "Radiance", atmosphere, "Radiance"));
    XII_SUCCEED_OR_RETURN(author.Connect(settings, "AtmosphereConstants", atmosphere, "Atmosphere"));
    XII_SUCCEED_OR_RETURN(author.Boundary("Depth", true, atmosphere, "Depth"));
    XII_SUCCEED_OR_RETURN(author.Boundary("EnvironmentLight", true, atmosphere, "EnvironmentLight", xiiRenderPipelinePin::Buffer));
    for (const xiiStringView pin : {"Transmittance", "MultiScatter"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, atmosphere, pin));
    XII_SUCCEED_OR_RETURN(author.Boundary("Radiance", false, atmosphere, "AtmosphereRadiance"));
    XII_SUCCEED_OR_RETURN(author.Boundary("After", false, atmosphere, "After", xiiRenderPipelinePin::Order));
    XII_SUCCEED_OR_RETURN(author.Boundary("ScreenIrradiance", false, diffuse, "Irradiance"));
    XII_SUCCEED_OR_RETURN(author.Boundary("ScreenLighting", false, selectGI, "Texture"));
    for (const xiiStringView pin : {"RefractionSignal", "TransmittedRadiance"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, false, refract, pin));
    return author.Save(path);
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeScreenTransportAuthoring);
XII_CREATE_SIMPLE_TEST(NativeScreenTransportAuthoring, ScreenAndAtmosphericResources)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "NativeScreenTransportAuthoring", "TransportBase", xiiDataDirUsage::ReadOnly)) || !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeScreenTransportAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeScreenTransportAuthoring"));
  XII_TEST_STATUS(AuthorTransportLeaf(path, "ScreenIrradiance", "xiiScreenIrradianceTracePass"));
  XII_TEST_STATUS(AuthorTransportLeaf(path, "ScreenRefractionTrace", "xiiScreenRefractionTracePass"));
  XII_TEST_STATUS(AuthorTransportLeaf(path, "AtmosphereApply", "xiiAtmosphereApplyPass"));
  XII_TEST_STATUS(AuthorScreenDiffuseLighting(path));
  XII_TEST_STATUS(AuthorScreenRefraction(path));
  XII_TEST_STATUS(AuthorScreenEffects(path));
}
