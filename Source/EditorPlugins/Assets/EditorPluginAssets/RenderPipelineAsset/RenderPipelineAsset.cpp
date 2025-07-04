#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <GraphicsCore/Pipeline/Extractor.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineAssetDocument, 5, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

bool xiiRenderPipelineNodeManager::InternalIsNode(const xiiDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<xiiRenderPipelinePass>() || pType->IsDerivedFrom<xiiExtractor>();
}

void xiiRenderPipelineNodeManager::InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& ref_node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<xiiRenderPipelinePass>())
    return;

  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() != xiiPropertyCategory::Member)
      continue;

    if (!pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodePin>())
      continue;

    xiiColor pinColor;
    if (const xiiColorAttribute* pColorAttribute = pProp->GetAttributeByType<xiiColorAttribute>())
    {
      pinColor = pColorAttribute->GetColor();
    }
    else
    {
      xiiColorScheme::Enum color = xiiColorScheme::Gray;

      if (pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodePassThroughPin>())
        color = xiiColorScheme::Gray;
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodeInputProviderPin>())
        color = xiiColorScheme::Orange;
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodeOutputProviderPin>())
        color = xiiColorScheme::Violet;
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodeInputPin>())
        color = xiiColorScheme::Blue;
      else if (pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodeOutputPin>())
        color = xiiColorScheme::Green;

      pinColor = xiiColorScheme::DarkUI(color);
    }

    if (pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodeInputPin>())
    {
      auto pPin = XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodeOutputPin>())
    {
      auto pPin = XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodePassThroughPin>())
    {
      auto pPinIn = XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Inputs.PushBack(pPinIn);

      auto pPinOut = XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Outputs.PushBack(pPinOut);
    }
  }
}

void xiiRenderPipelineNodeManager::GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const
{
  xiiSet<const xiiRTTI*> typeSet;
  xiiReflectionUtils::GatherTypesDerivedFromClass(xiiGetStaticRTTI<xiiRenderPipelinePass>(), typeSet);
  xiiReflectionUtils::GatherTypesDerivedFromClass(xiiGetStaticRTTI<xiiExtractor>(), typeSet);

  ref_types.Clear();

  for (auto pType : typeSet)
  {
    if (pType->GetTypeFlags().IsAnySet(xiiTypeFlags::Abstract))
      continue;

    ref_types.PushBack(pType);
  }
}

xiiStatus xiiRenderPipelineNodeManager::InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const
{
  out_result = CanConnectResult::ConnectNto1;
  return xiiStatus(XII_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////

xiiRenderPipelineAssetDocument::xiiRenderPipelineAssetDocument(xiiStringView sDocumentPath) :
  xiiAssetDocument(sDocumentPath, XII_DEFAULT_NEW(xiiRenderPipelineNodeManager), xiiAssetDocEngineConnection::FullObjectMirroring)
{
}

xiiRenderPipelineAssetDocument::~xiiRenderPipelineAssetDocument() = default;

xiiTransformStatus xiiRenderPipelineAssetDocument::InternalTransformAsset(xiiStringView sTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  return xiiAssetDocument::RemoteExport(AssetHeader, sTargetFile);
}

xiiTransformStatus xiiRenderPipelineAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  XII_REPORT_FAILURE("Should not be called");
  return xiiTransformStatus();
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

bool xiiRenderPipelineAssetDocument::Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType)
{
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, xiiQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
