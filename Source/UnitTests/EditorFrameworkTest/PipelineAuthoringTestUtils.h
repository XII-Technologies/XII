/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class xiiPipelineTestDocument : public xiiRenderPipelineAssetDocument
{
public:
  explicit xiiPipelineTestDocument(xiiStringView path, xiiStringView name = "PipelineAuthoring") : xiiRenderPipelineAssetDocument(path, xiiAssetDocEngineConnection::None)
  {
    m_pDocumentInfo               = CreateDocumentInfo();
    m_pDocumentInfo->m_DocumentID = xiiUuid::MakeStableUuidFromString(xiiStringBuilder("XII.RenderPipelines.", name));
    static xiiAssetDocumentTypeDescriptor descriptor;
    descriptor.m_sDocumentTypeName  = "RenderPipeline";
    descriptor.m_AssetDocumentFlags = xiiAssetDocumentFlags::DisableTransform;
    m_pTypeDescriptor               = &descriptor;
    static_cast<xiiAssetDocumentInfo*>(m_pDocumentInfo)->SetAssetsDocumentTypeName("RenderPipeline");
  }

protected:
  xiiTaskGroupID InternalSaveDocument(AfterSaveCallback callback) override { return xiiDocument::InternalSaveDocument(callback); }
  void           InternalAfterSaveDocument() override {}
};
class xiiPipelineTestContext : public xiiRenderPipelineContext
{
public:
  ~xiiPipelineTestContext()
  {
    for (const auto& id : m_Objects)
      m_Context.DeleteObject(id);
  }
  xiiResult Import(const xiiDocument& document)
  {
    m_DocumentGuid = document.GetGuid();
    xiiAbstractObjectGraph           graph;
    xiiDocumentObjectConverterWriter writer(&graph, document.GetObjectManager());
    for (const auto* object : document.GetObjectManager()->GetRootObject()->GetChildren())
      writer.AddObjectToGraph(object);
    xiiRttiConverterReader reader(&graph, &m_Context);
    for (const auto* object : document.GetObjectManager()->GetRootObject()->GetChildren())
    {
      if (!reader.CreateObjectFromNode(graph.GetNode(object->GetGuid())))
        return XII_FAILURE;
      m_Objects.PushBack(object->GetGuid());
    }
    return XII_SUCCESS;
  }
  using xiiRenderPipelineContext::ExportDocument;

private:
  xiiDynamicArray<xiiUuid> m_Objects;
};

class xiiPipelineTestAuthor
{
public:
  xiiPipelineTestAuthor(xiiStringView directory, xiiStringView name) : m_Name(name), m_Document(xiiStringBuilder(directory, "/RenderPipelines/", name, ".xiiRenderPipelineAsset"), name)
  {
    m_Accessor = m_Document.GetObjectAccessor();
    m_Nodes    = static_cast<xiiRenderPipelineNodeManager*>(m_Document.GetObjectManager());
    m_Accessor->StartTransaction("Author pipeline graph");
  }
  ~xiiPipelineTestAuthor()
  {
    if (!m_Finished)
      m_Accessor->CancelTransaction();
  }
  xiiStatus Init(xiiBitflags<xiiRenderPipelineDeviceFeature> features = xiiRenderPipelineDeviceFeature::Compute)
  {
    XII_SUCCEED_OR_RETURN(m_Document.EnsureSettingsObject());
    return m_Accessor->SetValueByName(m_Document.GetSettingsObject(), "RequiredDeviceFeatures", xiiInt64(features.GetValue()));
  }
  xiiStatus                Set(const xiiDocumentObject* node, xiiStringView property, const xiiVariant& value) { return node ? m_Accessor->SetValueByName(node, property, value) : xiiStatus("Missing authored pipeline node."); }
  xiiStatus                Settings(xiiStringView property, const xiiVariant& value) { return Set(m_Document.GetSettingsObject(), property, value); }
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
      return xiiStatus("Missing pipeline node.");
    const auto* out = m_Nodes->GetOutputPinByName(source, output);
    const auto* in  = m_Nodes->GetInputPinByName(target, input);
    if (!out || !in)
      return xiiStatus(xiiFmt("Missing pipeline pin '{} -> {}'.", output, input));
    return xiiNodeCommands::AddAndConnectCommand(m_Document.GetCommandHistory(), m_Nodes->GetConnectionType(), *out, *in);
  }
  xiiStatus Boundary(xiiStringView label, bool input, const xiiDocumentObject* node, xiiStringView pin, xiiRenderPipelinePin::Type kind = xiiRenderPipelinePin::Texture)
  {
    const auto* boundary = FindBoundary(label, input);
    if (!boundary)
    {
      boundary = Add(xiiStringBuilder(input ? "Input/" : "Output/", label), "xiiRenderPipelineInterfacePass", {input ? -700.0f : 2300.0f, float((input ? m_InputRow++ : m_OutputRow++) * 180)});
      if (!boundary)
        return xiiStatus("Missing pipeline boundary.");
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
