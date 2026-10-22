/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <EditorFrameworkTest/PipelineAuthoringTestUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Types/ScopeExit.h>
namespace
{
  xiiStatus AuthorAtmosphereLeaf(xiiStringView path, xiiStringView name, xiiStringView typeName)
  {
    const auto* type = xiiRTTI::FindTypeByName(typeName);
    if (!type)
      return xiiStatus("Missing atmosphere operation type.");
    xiiRenderPipelinePass* pass = type->GetAllocator()->Allocate<xiiRenderPipelinePass>();
    XII_SCOPE_EXIT(type->GetAllocator()->Deallocate(pass));
    xiiPipelineTestAuthor author(path, name);
    XII_SUCCEED_OR_RETURN(author.Init(pass->GetRequiredDeviceFeatures()));
    const auto*                           node = author.Add("Operation", typeName, {0, 0});
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    pass->GetPins(pins);
    for (const auto& pin : pins)
      XII_SUCCEED_OR_RETURN(author.Boundary(pin.m_sName, pin.m_bInput, node, pin.m_sName, pin.m_Type));
    return author.Save(path);
  }
  xiiStatus AuthorAtmosphereLookup(xiiStringView path)
  {
    xiiPipelineTestAuthor author(path, "AtmosphereLookup");
    XII_SUCCEED_OR_RETURN(author.Init());
    const auto* parameters   = author.Add("Physical Atmosphere Parameters", "xiiAtmosphereConstantsPass", {0, 0});
    const auto* transmission = author.Reference("Solar Transmittance", "AtmosphereTransmittance", {700, 0});
    const auto* scattering   = author.Reference("Multiple Scattering", "AtmosphereMultiScatter", {1400, 0});
    XII_SUCCEED_OR_RETURN(author.Connect(parameters, "AtmosphereConstants", transmission, "Atmosphere"));
    XII_SUCCEED_OR_RETURN(author.Connect(parameters, "AtmosphereConstants", scattering, "Atmosphere"));
    XII_SUCCEED_OR_RETURN(author.Connect(transmission, "Transmittance", scattering, "Transmittance"));
    XII_SUCCEED_OR_RETURN(author.Boundary("Atmosphere", false, parameters, "AtmosphereConstants", xiiRenderPipelinePin::Buffer));
    XII_SUCCEED_OR_RETURN(author.Boundary("Transmittance", false, transmission, "Transmittance"));
    XII_SUCCEED_OR_RETURN(author.Boundary("MultiScatter", false, scattering, "MultiScatter"));
    return author.Save(path);
  }
  xiiStatus AuthorWorldAtmosphereView(xiiStringView path)
  {
    xiiPipelineTestAuthor author(path, "WorldAtmosphereView");
    XII_SUCCEED_OR_RETURN(author.Init());
    const auto*              lookup = author.Reference("World Atmosphere Lookup", "AtmosphereLookup", {0, 0});
    const auto*              view   = author.Reference("Apply Atmospheric Radiance", "AtmosphereApply", {1200, 0});
    xiiGraphPropertyOverride override;
    override.m_Path.PushBack(xiiUuid::MakeStableUuidFromString("XII.RenderPipelines.AtmosphereLookup/Physical Atmosphere Parameters"));
    override.m_Properties.Insert("UseWorldAtmosphere", true);
    // Instance overrides are stored through the same reflected command accessor as all other properties.
    XII_SUCCEED_OR_RETURN(author.Override(lookup, override));
    for (const xiiStringView pin : {"Atmosphere", "Transmittance", "MultiScatter"})
      XII_SUCCEED_OR_RETURN(author.Connect(lookup, pin, view, pin));
    for (const xiiStringView pin : {"Depth", "Radiance"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, view, pin));
    for (const xiiStringView pin : {"Camera", "EnvironmentLight"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, view, pin, xiiRenderPipelinePin::Buffer));
    for (const xiiStringView pin : {"Transmittance", "MultiScatter"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, false, lookup, pin));
    XII_SUCCEED_OR_RETURN(author.Boundary("Atmosphere", false, lookup, "Atmosphere", xiiRenderPipelinePin::Buffer));
    XII_SUCCEED_OR_RETURN(author.Boundary("Radiance", false, view, "AtmosphereRadiance"));
    return author.Save(path);
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeAtmosphereAuthoring);
XII_CREATE_SIMPLE_TEST(NativeAtmosphereAuthoring, LookupAndWorldViewResources)
{
  const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "NativeAtmosphereAuthoring", "AtmosphereBase", xiiDataDirUsage::ReadOnly)) || !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeAtmosphereAuthoring", ":", xiiDataDirUsage::AllowWrites)))
    return;
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeAtmosphereAuthoring"));
  XII_TEST_STATUS(AuthorAtmosphereLeaf(path, "AtmosphereConstants", "xiiAtmosphereConstantsPass"));
  XII_TEST_STATUS(AuthorAtmosphereLeaf(path, "AtmosphereTransmittance", "xiiAtmosphereTransmittancePass"));
  XII_TEST_STATUS(AuthorAtmosphereLeaf(path, "AtmosphereMultiScatter", "xiiAtmosphereMultiScatterPass"));
  XII_TEST_STATUS(AuthorAtmosphereLookup(path));
  XII_TEST_STATUS(AuthorWorldAtmosphereView(path));
}
