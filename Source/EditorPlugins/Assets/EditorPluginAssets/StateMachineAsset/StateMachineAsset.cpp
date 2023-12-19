#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineAsset.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraph.h>
#include <GameEngine/StateMachine/StateMachineBuiltins.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineAssetDocument, 4, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachineAssetDocument::xiiStateMachineAssetDocument(xiiStringView sDocumentPath) :
  xiiAssetDocument(sDocumentPath, XII_DEFAULT_NEW(xiiStateMachineNodeManager), xiiAssetDocEngineConnection::None)
{
}

xiiTransformStatus xiiStateMachineAssetDocument::InternalTransformAsset(xiiStreamWriter& stream, xiiStringView sOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  auto pManager = static_cast<xiiStateMachineNodeManager*>(GetObjectManager());

  xiiAbstractObjectGraph           abstractObjectGraph;
  xiiDocumentObjectConverterWriter objectWriter(&abstractObjectGraph, pManager);
  xiiRttiConverterContext          converterContext;
  xiiRttiConverterReader           converter(&abstractObjectGraph, &converterContext);

  xiiStateMachineDescription                        desc;
  xiiHashTable<const xiiDocumentObject*, xiiUInt32> objectToStateIndex;
  xiiSet<xiiString>                                 stateNames;

  auto AddState = [&](const xiiDocumentObject* pObject) {
    xiiVariant nameVar = pObject->GetTypeAccessor().GetValue("Name");
    XII_ASSERT_DEV(nameVar.IsA<xiiString>(), "Implementation error");

    const xiiString& name = nameVar.Get<xiiString>();
    if (stateNames.Contains(name))
    {
      return xiiStatus(xiiFmt("A state named '{}' already exists. State names have to be unique.", name));
    }
    stateNames.Insert(name);

    xiiVariant type = pObject->GetTypeAccessor().GetValue("Type");
    XII_ASSERT_DEV(type.IsA<xiiUuid>(), "Implementation error");

    if (auto pStateObject = pObject->GetChild(type.Get<xiiUuid>()))
    {
      xiiAbstractObjectNode* pAbstractNode = objectWriter.AddObjectToGraph(pStateObject);
      auto                   pState        = converter.CreateObjectFromNode(pAbstractNode).Cast<xiiStateMachineState>();
      pState->SetName(name);

      const xiiUInt32 uiStateIndex = desc.AddState(pState);
      objectToStateIndex.Insert(pObject, uiStateIndex);
    }
    else
    {
      auto pState = XII_DEFAULT_NEW(xiiStateMachineState_Empty);
      pState->SetName(name);

      const xiiUInt32 uiStateIndex = desc.AddState(pState);
      objectToStateIndex.Insert(pObject, uiStateIndex);
    }

    return xiiStatus(XII_SUCCESS);
  };

  auto& allObjects = pManager->GetRootObject()->GetChildren();

  if (allObjects.IsEmpty() == false)
  {
    if (auto pObject = pManager->GetInitialState())
    {
      XII_SUCCEED_OR_RETURN(AddState(pObject));
      XII_ASSERT_DEV(objectToStateIndex[pObject] == 0, "Initial state has to have index 0");
    }
    else
    {
      return xiiStatus("Initial state is not set");
    }
  }

  for (const xiiDocumentObject* pObject : allObjects)
  {
    if (pManager->IsNode(pObject) == false || pManager->IsInitialState(pObject) || pManager->IsAnyState(pObject))
      continue;

    XII_SUCCEED_OR_RETURN(AddState(pObject));
  }

  for (const xiiDocumentObject* pObject : allObjects)
  {
    if (pManager->IsConnection(pObject) == false)
      continue;

    xiiVariant type = pObject->GetTypeAccessor().GetValue("Type");
    XII_ASSERT_DEV(type.IsA<xiiUuid>(), "Implementation error");

    xiiUniquePtr<xiiStateMachineTransition> pTransition;
    if (auto pTransitionObject = pObject->GetChild(type.Get<xiiUuid>()))
    {
      xiiAbstractObjectNode* pAbstractNode = objectWriter.AddObjectToGraph(pTransitionObject);
      pTransition                          = converter.CreateObjectFromNode(pAbstractNode).Cast<xiiStateMachineTransition>();
    }
    else
    {
      pTransition = XII_DEFAULT_NEW(xiiStateMachineTransition_Timeout);
    }

    const xiiConnection& connection       = pManager->GetConnection(pObject);
    xiiUInt32            uiFromStateIndex = xiiInvalidIndex;
    xiiUInt32            uiToStateIndex   = xiiInvalidIndex;
    if (pManager->IsAnyState(connection.GetSourcePin().GetParent()) == false)
    {
      XII_VERIFY(objectToStateIndex.TryGetValue(connection.GetSourcePin().GetParent(), uiFromStateIndex), "Implementation error");
    }
    XII_VERIFY(objectToStateIndex.TryGetValue(connection.GetTargetPin().GetParent(), uiToStateIndex), "Implementation error");

    desc.AddTransition(uiFromStateIndex, uiToStateIndex, std::move(pTransition));
  }

  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  XII_SUCCEED_OR_RETURN(desc.Serialize(stream));

  stream << storage.GetStorageSize32();
  return storage.CopyToStream(stream);
}

constexpr const char* s_szIsInitialState = "IsInitialState";

void xiiStateMachineAssetDocument::InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const
{
  auto pManager = static_cast<const xiiStateMachineNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);

  if (pManager->IsInitialState(pObject))
  {
    inout_uiHash = xiiHashingUtils::xxHash64String(s_szIsInitialState, inout_uiHash);
  }
}

void xiiStateMachineAssetDocument::AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const auto pManager = static_cast<const xiiStateMachineNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);

  if (auto pObject = pManager->GetInitialState())
  {
    xiiAbstractObjectNode* pAbstractObject = graph.GetNode(pObject->GetGuid());
    pAbstractObject->AddProperty(s_szIsInitialState, true);
  }
}

void xiiStateMachineAssetDocument::RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  auto pManager = static_cast<xiiStateMachineNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);

  for (auto it : graph.GetAllNodes())
  {
    auto pAbstractObject = it.Value();
    if (auto pProperty = pAbstractObject->FindProperty(s_szIsInitialState))
    {
      if (pProperty->m_Value.ConvertTo<bool>() == false)
        continue;

      xiiDocumentObject* pObject = pManager->GetObject(pAbstractObject->GetGuid());
      pManager->SetInitialState(pObject);
    }
  }
}

void xiiStateMachineAssetDocument::GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/xiiEditor.StateMachineGraph");
}

bool xiiStateMachineAssetDocument::CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const
{
  out_MimeType = "application/xiiEditor.StateMachineGraph";

  const xiiDocumentNodeManager* pManager = static_cast<const xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool xiiStateMachineAssetDocument::Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, xiiStringView sMimeType)
{
  xiiDocumentNodeManager* pManager = static_cast<xiiDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, xiiQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
