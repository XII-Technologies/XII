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
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodeOutputPin>())
    {
      auto pPin = XII_DEFAULT_NEW(xiiPin, xiiPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<xiiRenderPipelineNodePassThrougPin>())
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

xiiRenderPipelineAssetDocument::xiiRenderPipelineAssetDocument(xiiStringView sDocumentPath) :
  xiiAssetDocument(sDocumentPath, XII_DEFAULT_NEW(xiiRenderPipelineNodeManager), xiiAssetDocEngineConnection::FullObjectMirroring)
{
}

xiiRenderPipelineAssetDocument::~xiiRenderPipelineAssetDocument()
{
  static_cast<xiiRenderPipelineObjectMirrorEditor*>(m_pMirror.Borrow())->DeInitNodeSender();
}


void xiiRenderPipelineAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  m_pMirror = XII_DEFAULT_NEW(xiiRenderPipelineObjectMirrorEditor);
  static_cast<xiiRenderPipelineObjectMirrorEditor*>(m_pMirror.Borrow())->InitNodeSender(static_cast<const xiiDocumentNodeManager*>(GetObjectManager()));
}

xiiTransformStatus xiiRenderPipelineAssetDocument::InternalTransformAsset(const char* szTargetFile, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  return xiiAssetDocument::RemoteExport(AssetHeader, szTargetFile);
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

void xiiRenderPipelineObjectMirrorEditor::InitNodeSender(const xiiDocumentNodeManager* pNodeManager)
{
  m_pNodeManager = pNodeManager;
  m_pNodeManager->m_NodeEvents.AddEventHandler(xiiMakeDelegate(&xiiRenderPipelineObjectMirrorEditor::NodeEventsHandler, this));
}

void xiiRenderPipelineObjectMirrorEditor::DeInitNodeSender()
{
  m_pNodeManager->m_NodeEvents.RemoveEventHandler(xiiMakeDelegate(&xiiRenderPipelineObjectMirrorEditor::NodeEventsHandler, this));
}

void xiiRenderPipelineObjectMirrorEditor::ApplyOp(xiiObjectChange& ref_change)
{
  // SUPER::ApplyOp will move the data out of the payload, so we have to check for connections before.
  const xiiConnection* pConnection = nullptr;
  if (ref_change.m_Change.m_Operation == xiiObjectChangeType::NodeAdded)
  {
    const xiiDocumentObject* pObject = m_pNodeManager->GetObject(ref_change.m_Change.m_Value.Get<xiiUuid>());
    if (pObject != nullptr && m_pNodeManager->IsConnection(pObject))
    {
      pConnection = &m_pNodeManager->GetConnection(pObject);
    }
  }
  SUPER::ApplyOp(ref_change);

  // We need to handle this case in addition to the NodeEventsHandler because after loading the meta data is restored before the object mirror is initialized so we miss all the NodeEventsHandler calls.
  if (pConnection)
    SendConnection(*pConnection);
}

void xiiRenderPipelineObjectMirrorEditor::NodeEventsHandler(const xiiDocumentNodeManagerEvent& e)
{
  if (e.m_EventType == xiiDocumentNodeManagerEvent::Type::AfterPinsConnected)
  {
    const xiiConnection& connection = m_pNodeManager->GetConnection(e.m_pObject);
    SendConnection(connection);
  }
}

void xiiRenderPipelineObjectMirrorEditor::SendConnection(const xiiConnection& connection)
{
  const xiiPin& sourcePin = connection.GetSourcePin();
  const xiiPin& targetPin = connection.GetTargetPin();

  xiiUuid   Source    = sourcePin.GetParent()->GetGuid();
  xiiUuid   Target    = targetPin.GetParent()->GetGuid();
  xiiString SourcePin = sourcePin.GetName();
  xiiString TargetPin = targetPin.GetName();

  auto SendMetaData = [this](const xiiDocumentObject* pObject, const char* szProperty, xiiVariant value) {
    xiiObjectChange change;
    CreatePath(change, pObject, szProperty);
    change.m_Change.m_Operation = xiiObjectChangeType::PropertySet;
    change.m_Change.m_Value     = value;
    ApplyOp(change);
  };
  SendMetaData(connection.GetParent(), "Connection::Source", Source);
  SendMetaData(connection.GetParent(), "Connection::Target", Target);
  SendMetaData(connection.GetParent(), "Connection::SourcePin", SourcePin);
  SendMetaData(connection.GetParent(), "Connection::TargetPin", TargetPin);
}
