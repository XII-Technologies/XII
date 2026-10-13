/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAsset.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphQt.h>
#include <Foundation/IO/MemoryStream.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <SharedPluginAssets/AnimationGraphAsset/AnimationGraphAssetCompiler.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>

namespace
{
  void NodePropertyMetaState(xiiPropertyMetaStateEvent& event)
  {
    if (!event.m_pObject->GetType()->IsDerivedFrom<xiiAnimationGraphEditorNode>())
      return;
    const auto kind    = event.m_pObject->GetTypeAccessor().GetValue("Kind").ConvertTo<xiiInt64>();
    auto&      states  = *event.m_pPropertyStates;
    auto       visible = [&](const char* property, bool show) {
      states[property].m_Visibility = show ? xiiPropertyUiState::Default : xiiPropertyUiState::Invisible;
    };
    visible("Clip", kind == xiiAnimationGraphNodeKind::Clip);
    visible("PlaybackSpeed", kind == xiiAnimationGraphNodeKind::Clip);
    visible("Parameter", kind == xiiAnimationGraphNodeKind::Blend1D || kind == xiiAnimationGraphNodeKind::LayeredBlend);
    visible("Weight", kind == xiiAnimationGraphNodeKind::Additive || kind == xiiAnimationGraphNodeKind::LayeredBlend);
    visible("TargetJoint", kind == xiiAnimationGraphNodeKind::LayeredBlend);
    visible("StateCount", kind == xiiAnimationGraphNodeKind::StateMachine);
    visible("BlendThresholds", kind == xiiAnimationGraphNodeKind::Blend1D);
  }
} // namespace

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationGraphAssetDocument, 6, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationGraphAssetProperties, 2, xiiRTTIDefaultAllocator<xiiAnimationGraphAssetProperties>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Transitions", m_Transitions),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, AnimationGraph)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core",
    "PropertyMetaState"
  END_SUBSYSTEM_DEPENDENCIES
  ON_CORESYSTEMS_STARTUP
  {
    xiiQtNodeScene::GetNodeFactory().RegisterCreator(xiiGetStaticRTTI<xiiAnimationGraphEditorNode>(), [](const xiiRTTI*) -> xiiQtNode* { return new xiiQtAnimationGraphNode(); });
    xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(NodePropertyMetaState);
  }
  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(NodePropertyMetaState);
    xiiQtNodeScene::GetNodeFactory().UnregisterCreator(xiiGetStaticRTTI<xiiAnimationGraphEditorNode>());
  }
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiAnimationGraphAssetDocument::xiiAnimationGraphAssetDocument(xiiStringView path) :
  xiiSimpleAssetDocument<xiiAnimationGraphAssetProperties>(XII_DEFAULT_NEW(xiiAnimationGraphNodeManager), path, xiiAssetDocEngineConnection::None)
{
  m_pObjectAccessor = XII_DEFAULT_NEW(xiiNodeCommandAccessor, GetCommandHistory());
}

xiiStatus xiiAnimationGraphAssetDocument::InternalLoadDocument()
{
  // Reject the removed trigger/number/bone-mask graph format before phantom type
  // registration can load it as a partially functional document. The source file
  // remains intact; unsupported semantics must not be silently stripped on save.
  xiiUniquePtr<xiiAbstractObjectGraph> header, objects, types;
  XII_SUCCEED_OR_RETURN(ReadDocument(GetDocumentPath(), header, objects, types));
  for (auto it = objects->GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    const auto* node = it.Value();
    if (node->GetType() == "xiiAnimationGraphAssetProperties" && node->GetTypeVersion() < 2)
      return xiiStatus("This animation graph uses the removed pin-based runtime. Its source file is unchanged. Migrate supported pose operations to a native graph; trigger, numeric and included-graph semantics have no native replacement.");
  }
  return SUPER::InternalLoadDocument();
}

xiiTransformStatus xiiAnimationGraphAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  const auto* manager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  if (GetUnknownObjectTypeInstances() != 0)
    return xiiStatus("Animation graph contains unknown object types.");
  xiiAnimGraphResourceDescriptor compiled;
  xiiStringBuilder               error;
  if (xiiAnimationGraphAssetCompiler::CompileDocument(*manager, GetProperties()->m_Transitions, compiled, &error).Failed())
    return xiiStatus(error.GetView());
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  XII_SUCCEED_OR_RETURN(compiled.Serialize(writer));
  return storage.CopyToStream(stream);
}

void xiiAnimationGraphAssetDocument::InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const
{
  // without this, changing connections only (no property value) may not result in a different asset document hash and therefore no transform

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void xiiAnimationGraphAssetDocument::AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void xiiAnimationGraphAssetDocument::RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}



void xiiAnimationGraphAssetDocument::GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/xiiEditor.AnimationGraphGraph");
}

bool xiiAnimationGraphAssetDocument::CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const
{
  out_MimeType = "application/xiiEditor.AnimationGraphGraph";

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool xiiAnimationGraphAssetDocument::Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType)
{
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, xiiQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
