#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineAssetDocument, 4, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

bool xiiRenderPipelineNodeManager::InternalIsNode(const xiiDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<xiiRenderPipelinePass>() || pType->IsDerivedFrom<xiiExtractor>();
}

void xiiRenderPipelineNodeManager::InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<xiiRenderPipelinePass>())
    return;

  xiiHybridArray<xiiAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (xiiAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != xiiPropertyCategory::Member)
      continue;

    if (!pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodePin>())
      continue;

    xiiColor pinColor;
    if (const xiiColorAttribute* pAttr = pProp->GetAttributeByType<xiiColorAttribute>())
    {
      pinColor = pAttr->GetColor();
    }
    else
    {
      xiiColorScheme::Enum color = xiiColorScheme::Gray;
      if (xiiStringUtils::IsEqual(pProp->GetPropertyName(), "DepthStencil"))
        color = xiiColorScheme::Pink;

      pinColor = xiiColorScheme::DarkUI(color);
    }

    if (pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodeInputPin>())
    {
      auto pPin = XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodeOutputPin>())
    {
      auto pPin = XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodePassThrougPin>())
    {
      auto pPinIn = XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Inputs.PushBack(pPinIn);
      auto pPinOut = XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Outputs.PushBack(pPinOut);
    }
  }
}

void xiiRenderPipelineNodeManager::GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& Types) const
{
  xiiSet<const xiiRTTI*> typeSet;
  xiiReflectionUtils::GatherTypesDerivedFromClass(xiiGetStaticRTTI<xiiRenderPipelinePass>(), typeSet, false);
  xiiReflectionUtils::GatherTypesDerivedFromClass(xiiGetStaticRTTI<xiiExtractor>(), typeSet, false);
  Types.Clear();
  for (auto pType : typeSet)
  {
    if (pType->GetTypeFlags().IsAnySet(xiiTypeFlags::Abstract))
      continue;

    Types.PushBack(pType);
  }
}

xiiStatus xiiRenderPipelineNodeManager::InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNto1;
  return xiiStatus(XII_SUCCESS);
}

xiiRenderPipelineAssetDocument::xiiRenderPipelineAssetDocument(const char* szDocumentPath) :
  xiiAssetDocument(szDocumentPath, XII_DEFAULT_NEW(xiiRenderPipelineNodeManager), xiiAssetDocEngineConnection::None)
{
}

xiiTransformStatus xiiRenderPipelineAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());

  const xiiUInt8 uiVersion = 1;
  stream << uiVersion;

  xiiAbstractObjectGraph           graph;
  xiiDocumentObjectConverterWriter objectConverter(&graph, GetObjectManager());

  auto& children = GetObjectManager()->GetRootObject()->GetChildren();
  for (xiiDocumentObject* pObject : children)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    if (pType->IsDerivedFrom<xiiRenderPipelinePass>())
    {
      objectConverter.AddObjectToGraph(pObject, "Pass");
    }
    else if (pType->IsDerivedFrom<xiiExtractor>())
    {
      objectConverter.AddObjectToGraph(pObject, "Extractor");
    }
    else if (pManager->IsConnection(pObject))
    {
      objectConverter.AddObjectToGraph(pObject, "Connection");
    }
  }

  pManager->AttachMetaDataBeforeSaving(graph);

  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  xiiAbstractGraphBinarySerializer::Write(writer, &graph);

  xiiUInt32 uiSize = storage.GetStorageSize32();
  stream << uiSize;
  return storage.CopyToStream(stream);
}

void xiiRenderPipelineAssetDocument::InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const
{
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void xiiRenderPipelineAssetDocument::AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void xiiRenderPipelineAssetDocument::RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}



void xiiRenderPipelineAssetDocument::GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/xiiEditor.RenderPipelineGraph");
}

bool xiiRenderPipelineAssetDocument::CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const
{
  out_MimeType = "application/xiiEditor.RenderPipelineGraph";

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool xiiRenderPipelineAssetDocument::Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, xiiQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
