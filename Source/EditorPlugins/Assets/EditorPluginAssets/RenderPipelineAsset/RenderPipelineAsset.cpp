/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

class xiiRenderPipelinePinColorizer
{
public:
  static xiiColor Colorize(const xiiRTTI* pRtti)
  {
    if (pRtti == nullptr)
      return xiiColorScheme::DarkUI(xiiColorScheme::Gray);

    if (pRtti->IsDerivedFrom<xiiRenderPipelineNodeInputProviderPin>() || pRtti->IsDerivedFrom<xiiRenderPipelineNodeOutputProviderPin>())
    {
      if (pRtti->IsDerivedFrom<xiiRenderPipelineNodeInputProviderPin>())
        return xiiColorScheme::DarkUI(xiiColorScheme::Orange);

      return xiiColorScheme::DarkUI(xiiColorScheme::Violet);
    }

    if (pRtti->IsDerivedFrom<xiiRenderPipelineNodePassThroughPin>())
      return xiiColorScheme::DarkUI(xiiColorScheme::Gray);

    if (pRtti->IsDerivedFrom<xiiRenderPipelineNodeInputPin>())
      return xiiColorScheme::DarkUI(xiiColorScheme::Blue);

    if (pRtti->IsDerivedFrom<xiiRenderPipelineNodeOutputPin>())
      return xiiColorScheme::DarkUI(xiiColorScheme::Green);

    return xiiColorScheme::DarkUI(xiiColorScheme::Gray);
  }
};

bool xiiRenderPipelineNodeManager::InternalIsNode(const xiiDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();

  return pType->IsDerivedFrom<xiiRenderPipelinePassBase>() || pType->IsDerivedFrom<xiiExtractor>();
}

void xiiRenderPipelineNodeManager::InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& ref_node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<xiiRenderPipelinePassBase>())
    return;

  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (auto pProperty : properties)
  {
    if (pProperty->GetCategory() != xiiPropertyCategory::Member)
      continue;

    if (!pProperty->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodePin>())
      continue;

    xiiColor pinColor;
    if (const xiiColorAttribute* pColorAttribute = pProperty->GetAttributeByType<xiiColorAttribute>())
    {
      pinColor = pColorAttribute->GetColor();
    }
    else
    {
      pinColor = xiiRenderPipelinePinColorizer::Colorize(pProperty->GetSpecificType());
    }

    if (pProperty->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodeInputPin>())
    {
      auto pPin = XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Input, pProperty->GetPropertyName(), pinColor, pObject);
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pProperty->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodeOutputPin>())
    {
      auto pPin = XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Output, pProperty->GetPropertyName(), pinColor, pObject);
      ref_node.m_Outputs.PushBack(pPin);
    }
    else if (pProperty->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodePassThroughPin>())
    {
      auto pPinIn = XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Input, pProperty->GetPropertyName(), pinColor, pObject);
      ref_node.m_Inputs.PushBack(pPinIn);

      auto pPinOut = XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Output, pProperty->GetPropertyName(), pinColor, pObject);
      ref_node.m_Outputs.PushBack(pPinOut);
    }
  }
}

void xiiRenderPipelineNodeManager::GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const
{
  xiiSet<const xiiRTTI*> typeSet;
  xiiReflectionUtils::GatherTypesDerivedFromClass(xiiGetStaticRTTI<xiiRenderPipelinePassBase>(), typeSet);
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

  return XII_SUCCESS;
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
