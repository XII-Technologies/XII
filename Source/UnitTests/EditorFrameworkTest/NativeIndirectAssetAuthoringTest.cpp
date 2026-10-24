/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <EditorFrameworkTest/PipelineAuthoringTestUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
namespace
{
  xiiStatus AuthorLeaf(xiiStringView directory, xiiStringView name, xiiStringView typeName)
  {
    xiiPipelineTestAuthor author(directory, name);
    XII_SUCCEED_OR_RETURN(author.Init());
    const auto* node = author.Add("Evaluate", typeName, {700, 0});
    if (!node)
      return xiiStatus("Missing reflected indirect operation.");
    const auto*            type      = xiiRTTI::FindTypeByName(typeName);
    xiiRenderPipelinePass* prototype = type->GetAllocator()->Allocate<xiiRenderPipelinePass>();
    XII_SCOPE_EXIT(type->GetAllocator()->Deallocate(prototype));
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    prototype->GetPins(pins);
    for (const auto& pin : pins)
      XII_SUCCEED_OR_RETURN(author.Boundary(pin.m_sName, pin.m_bInput, node, pin.m_sName, pin.m_Type));
    return author.Save(directory);
  }
  xiiStatus AuthorLayeredDiffuse(xiiStringView directory)
  {
    xiiPipelineTestAuthor author(directory, "LayeredDiffuseLighting");
    XII_SUCCEED_OR_RETURN(author.Init());
    const auto* layer   = author.Add("Combine Coarse And Detail Confidence", "xiiIrradianceLayerPass", {0, 0});
    const auto* diffuse = author.Add("Evaluate Diffuse Material", "xiiDiffuseSurfaceLightingPass", {900, 0});
    XII_SUCCEED_OR_RETURN(author.Connect(layer, "Irradiance", diffuse, "Irradiance"));
    XII_SUCCEED_OR_RETURN(author.Boundary("CoarseIrradiance", true, layer, "CoarseIrradiance"));
    XII_SUCCEED_OR_RETURN(author.Boundary("DetailIrradiance", true, layer, "DetailIrradiance"));
    for (const xiiStringView pin : {"Depth", "Albedo", "Material", "Emissive", "Occlusion"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, diffuse, pin));
    XII_SUCCEED_OR_RETURN(author.Boundary("EnvironmentLight", true, diffuse, "EnvironmentLight", xiiRenderPipelinePin::Buffer));
    XII_SUCCEED_OR_RETURN(author.Boundary("DiffuseLighting", false, diffuse, "Lighting"));
    XII_SUCCEED_OR_RETURN(author.Boundary("CombinedIrradiance", false, layer, "Irradiance"));
    return author.Save(directory);
  }
  xiiStatus AuthorSurfaceIndirect(xiiStringView directory)
  {
    xiiPipelineTestAuthor author(directory, "SurfaceIndirectLighting");
    XII_SUCCEED_OR_RETURN(author.Init());
    const auto* diffuse  = author.Reference("Layered Diffuse", xiiCreateLayeredDiffuseLightingTestResource(), {0, 0});
    const auto* specular = author.Reference("Environment Specular", xiiCreateEnvironmentSpecularTestResource(), {0, 900});
    const auto* sum      = author.Add("Combine Indirect Contributions", "xiiRadianceAddPass", {1400, 0});
    XII_SUCCEED_OR_RETURN(author.Connect(diffuse, "DiffuseLighting", sum, "A"));
    XII_SUCCEED_OR_RETURN(author.Connect(specular, "Lighting", sum, "B"));
    for (const xiiStringView pin : {"Depth", "Albedo", "Material", "Occlusion"})
    {
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, diffuse, pin));
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, specular, pin));
    }
    for (const xiiStringView pin : {"Emissive", "CoarseIrradiance", "DetailIrradiance"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, diffuse, pin));
    XII_SUCCEED_OR_RETURN(author.Boundary("EnvironmentLight", true, diffuse, "EnvironmentLight", xiiRenderPipelinePin::Buffer));
    for (const xiiStringView pin : {"Normal", "EnvironmentRadiance", "BRDFLut"})
      XII_SUCCEED_OR_RETURN(author.Boundary(pin, true, specular, pin));
    XII_SUCCEED_OR_RETURN(author.Boundary("Camera", true, specular, "Camera", xiiRenderPipelinePin::Buffer));
    XII_SUCCEED_OR_RETURN(author.Boundary("DiffuseLighting", false, diffuse, "DiffuseLighting"));
    XII_SUCCEED_OR_RETURN(author.Boundary("EnvironmentSpecular", false, specular, "Lighting"));
    XII_SUCCEED_OR_RETURN(author.Boundary("IndirectLighting", false, sum, "Radiance"));
    return author.Save(directory);
  }
  template <typename F> void WithOutput(F&& function)
  {
    const auto path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
    if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "NativeIndirectAuthoring", "IndirectBase", xiiDataDirUsage::ReadOnly)) || !XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "NativeIndirectAuthoring", ":", xiiDataDirUsage::AllowWrites)))
      return;
    XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("NativeIndirectAuthoring"));
    XII_TEST_STATUS(function(path));
  }
} // namespace
XII_CREATE_SIMPLE_TEST_GROUP(NativeIndirectAuthoring);
XII_CREATE_SIMPLE_TEST(NativeIndirectAuthoring, DiffuseSurfaceLighting)
{
  WithOutput([](xiiStringView path) { return AuthorLeaf(path, "DiffuseSurfaceLighting", "xiiDiffuseSurfaceLightingPass"); });
}
XII_CREATE_SIMPLE_TEST(NativeIndirectAuthoring, EnvironmentSpecular)
{
  WithOutput([](xiiStringView path) { return AuthorLeaf(path, "EnvironmentSpecular", "xiiEnvironmentSpecularPass"); });
}
XII_CREATE_SIMPLE_TEST(NativeIndirectAuthoring, IrradianceLayer)
{
  WithOutput([](xiiStringView path) { return AuthorLeaf(path, "IrradianceLayer", "xiiIrradianceLayerPass"); });
}
XII_CREATE_SIMPLE_TEST(NativeIndirectAuthoring, LayeredDiffuseLighting) { WithOutput(AuthorLayeredDiffuse); }
XII_CREATE_SIMPLE_TEST(NativeIndirectAuthoring, SurfaceIndirectLighting) { WithOutput(AuthorSurfaceIndirect); }
