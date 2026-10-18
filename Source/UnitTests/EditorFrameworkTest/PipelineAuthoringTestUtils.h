/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
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
