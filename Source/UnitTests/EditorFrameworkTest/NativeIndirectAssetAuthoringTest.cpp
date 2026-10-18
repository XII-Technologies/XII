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
  class IndirectAuthor
  {
  public:
    IndirectAuthor(xiiStringView directory, xiiStringView name) : m_Name(name), m_Document(xiiStringBuilder(directory, "/RenderPipelines/", name, ".xiiRenderPipelineAsset"), name)
    {
      m_Accessor = m_Document.GetObjectAccessor();
      m_Nodes    = static_cast<xiiRenderPipelineNodeManager*>(m_Document.GetObjectManager());
      m_Accessor->StartTransaction("Author independent indirect illumination");
    }
    ~IndirectAuthor()
    {
      if (!m_Finished)
        m_Accessor->CancelTransaction();
    }
    xiiStatus Init()
    {
      XII_SUCCEED_OR_RETURN(m_Document.EnsureSettingsObject());
      return m_Accessor->SetValueByName(m_Document.GetSettingsObject(), "RequiredDeviceFeatures", xiiInt64(xiiRenderPipelineDeviceFeature::Compute));
    }
    const xiiDocumentObject* Add(xiiStringView label, xiiStringView type, xiiVec2 position)
    {
      auto id = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.", m_Name, "/", label));
      if (m_Accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName(type), id).Failed())
        return nullptr;
      xiiMoveNodeCommand move;
      move.m_Object = id;
      move.m_NewPos = position;
      if (m_Document.GetCommandHistory()->AddCommand(move).Failed())
        return nullptr;
      return m_Accessor->GetObject(id);
    }
    const xiiDocumentObject* Reference(xiiStringView label, xiiStringView name, xiiVec2 position)
    {
      const auto* node = Add(label, "xiiRenderPipelineSubgraphPass", position);
      if (!node || m_Accessor->SetValueByName(node, "Resource", xiiStringBuilder("RenderPipelines/", name, ".xiiBinRenderPipeline").GetView()).Failed() || m_Nodes->SynchronizePinConfigurations(node).Failed())
        return nullptr;
      return node;
    }
    xiiStatus Connect(const xiiDocumentObject* source, xiiStringView output, const xiiDocumentObject* target, xiiStringView input)
    {
      if (!source || !target)
        return xiiStatus("Missing indirect illumination node.");
      const auto* out = m_Nodes->GetOutputPinByName(source, output);
      const auto* in  = m_Nodes->GetInputPinByName(target, input);
      if (!out || !in)
        return xiiStatus(xiiFmt("Missing indirect illumination pin '{} -> {}'.", output, input));
      return xiiNodeCommands::AddAndConnectCommand(m_Document.GetCommandHistory(), m_Nodes->GetConnectionType(), *out, *in);
    }
    xiiStatus Boundary(xiiStringView label, bool input, const xiiDocumentObject* node, xiiStringView pin, xiiRenderPipelinePin::Type kind = xiiRenderPipelinePin::Texture)
    {
      const auto* boundary = FindBoundary(label, input);
      if (!boundary)
      {
        boundary = Add(xiiStringBuilder(input ? "Input/" : "Output/", label), "xiiRenderPipelineInterfacePass", {input ? -700.0f : 2300.0f, float((input ? m_InputRow++ : m_OutputRow++) * 180)});
        if (!boundary)
          return xiiStatus("Missing indirect illumination boundary.");
        XII_SUCCEED_OR_RETURN(m_Accessor->SetValueByName(boundary, "Name", label));
        XII_SUCCEED_OR_RETURN(m_Accessor->SetValueByName(boundary, "Input", input));
        XII_SUCCEED_OR_RETURN(m_Accessor->SetValueByName(boundary, "ResourceType", xiiUInt32(kind)));
        XII_SUCCEED_OR_RETURN(m_Nodes->SynchronizePinConfigurations(boundary));
      }
      return input ? Connect(boundary, "Value", node, pin) : Connect(node, pin, boundary, "Value");
    }
    xiiStatus Save(xiiStringView directory)
    {
      m_Accessor->FinishTransaction();
      m_Finished = true;
      XII_SUCCEED_OR_RETURN(m_Document.SaveDocument(true));
      xiiPipelineTestContext context;
      XII_SUCCEED_OR_RETURN(context.Import(m_Document));
      xiiExportDocumentMsgToEngine message;
      message.m_sOutputFile = xiiStringBuilder(directory, "/RenderPipelines/", m_Name, ".xiiBinRenderPipeline");
      message.m_uiAssetHash = 1;
      message.m_uiVersion   = 7;
      return context.ExportDocument(&message);
    }

  private:
    const xiiDocumentObject*      FindBoundary(xiiStringView label, bool input) const { return m_Accessor->GetObject(xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.", m_Name, "/", input ? "Input/" : "Output/", label))); }
    xiiString                     m_Name;
    xiiPipelineTestDocument       m_Document;
    xiiObjectAccessorBase*        m_Accessor = nullptr;
    xiiRenderPipelineNodeManager* m_Nodes    = nullptr;
    xiiUInt32                     m_InputRow = 0, m_OutputRow = 0;
    bool                          m_Finished = false;
  };
  xiiStatus AuthorLeaf(xiiStringView directory, xiiStringView name, xiiStringView typeName)
  {
    IndirectAuthor author(directory, name);
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
      if (pin.m_Type != xiiRenderPipelinePin::Order)
        XII_SUCCEED_OR_RETURN(author.Boundary(pin.m_sName, pin.m_bInput, node, pin.m_sName, pin.m_Type));
    return author.Save(directory);
  }
  xiiStatus AuthorLayeredDiffuse(xiiStringView directory)
  {
    IndirectAuthor author(directory, "LayeredDiffuseLighting");
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
    IndirectAuthor author(directory, "SurfaceIndirectLighting");
    XII_SUCCEED_OR_RETURN(author.Init());
    const auto* diffuse  = author.Reference("Layered Diffuse", "LayeredDiffuseLighting", {0, 0});
    const auto* specular = author.Reference("Environment Specular", "EnvironmentSpecular", {0, 900});
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
