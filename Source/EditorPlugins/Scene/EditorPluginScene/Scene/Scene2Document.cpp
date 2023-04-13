#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneLayerBase, 1, xiiRTTINoAllocator)
{
  //XII_BEGIN_PROPERTIES
  //{
  //}
  //XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneLayer, 1, xiiRTTIDefaultAllocator<xiiSceneLayer>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Layer", m_Layer)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneDocumentSettings, 2, xiiRTTIDefaultAllocator<xiiSceneDocumentSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Layers", m_Layers)->AddFlags(xiiPropertyFlags::PointerOwner)
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScene2Document, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


xiiSceneLayerBase::xiiSceneLayerBase()
{
}

xiiSceneLayerBase::~xiiSceneLayerBase()
{
}

//////////////////////////////////////////////////////////////////////////

xiiSceneLayer::xiiSceneLayer()
{
}

xiiSceneLayer::~xiiSceneLayer()
{
}

//////////////////////////////////////////////////////////////////////////

xiiSceneDocumentSettings::xiiSceneDocumentSettings()
{
}

xiiSceneDocumentSettings::~xiiSceneDocumentSettings()
{
  for (xiiSceneLayerBase* pLayer : m_Layers)
  {
    XII_DEFAULT_DELETE(pLayer);
  }
}

xiiScene2Document::xiiScene2Document(const char* szDocumentPath) :
  xiiSceneDocument(szDocumentPath, xiiSceneDocument::DocumentType::Scene)
{
  // Separate selection for the layer panel.
  m_pLayerSelection = XII_DEFAULT_NEW(xiiSelectionManager, m_pObjectManager.Borrow());
}

xiiScene2Document::~xiiScene2Document()
{
  SetActiveLayer(GetGuid()).LogFailure();

  // We need to clear all things that are dependent in the current object manager, selection etc setup before we swap the managers as otherwise those will fail to de-register.
  xiiVisualizerManager::GetSingleton()->SetVisualizersActive(this, false);
  m_pSelectionManager->Clear();

  // Game object document subscribed to the true document originally but we rerouted that to the mock data.
  // In order to destroy the game object document we need to revert this and subscribe to the true document again.
  UnsubscribeGameObjectEventHandlers();

  // Move the preserved real scene document back.
  m_pSelectionManager      = std::move(m_pSceneSelectionManager);
  m_pCommandHistory        = std::move(m_pSceneCommandHistory);
  m_pObjectManager         = std::move(m_pSceneObjectManager);
  m_pObjectAccessor        = std::move(m_pSceneObjectAccessor);
  m_DocumentObjectMetaData = std::move(m_pSceneDocumentObjectMetaData);
  m_GameObjectMetaData     = std::move(m_pSceneGameObjectMetaData);

  // See comment above for UnsubscribeGameObjectEventHandlers.
  SubscribeGameObjectEventHandlers();

  m_DocumentManagerEventSubscriber.Unsubscribe();
  m_LayerSelectionEventSubscriber.Unsubscribe();
  m_StructureEventSubscriber.Unsubscribe();
  m_CommandHistoryEventSubscriber.Unsubscribe();

  m_pLayerSelection = nullptr;

  for (auto it : m_Layers)
  {
    auto pDoc = it.Value().m_pLayer;

    if (pDoc && pDoc != this)
    {
      xiiDocumentManager* pManager = pDoc->GetDocumentManager();
      pManager->CloseDocument(pDoc);
    }
  }
}

void xiiScene2Document::InitializeAfterLoading(bool bFirstTimeCreation)
{
  EnsureSettingsObjectExist();

  m_ActiveLayerGuid = GetGuid();
  xiiObjectDirectAccessor accessor(GetObjectManager());
  xiiObjectAccessorBase*  pAccessor = &accessor;
  auto                    pRoot     = GetObjectManager()->GetObject(GetSettingsObject()->GetGuid());
  if (pRoot->GetChildren().IsEmpty())
  {
    xiiUuid objectGuid;
    pAccessor->AddObject(pRoot, "Layers", 0, xiiGetStaticRTTI<xiiSceneLayer>(), objectGuid);
    const xiiDocumentObject* pObject = pAccessor->GetObject(objectGuid);
    pAccessor->SetValue(pObject, "Layer", GetGuid());
  }

  SUPER::InitializeAfterLoading(bFirstTimeCreation);
}

void xiiScene2Document::InitializeAfterLoadingAndSaving()
{
  m_pLayerSelection->m_Events.AddEventHandler(xiiMakeDelegate(&xiiScene2Document::LayerSelectionEventHandler, this), m_LayerSelectionEventSubscriber);
  m_pObjectManager->m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiScene2Document::StructureEventHandler, this), m_StructureEventSubscriber);
  m_pCommandHistory->m_Events.AddEventHandler(xiiMakeDelegate(&xiiScene2Document::CommandHistoryEventHandler, this), m_CommandHistoryEventSubscriber);
  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiScene2Document::DocumentManagerEventHandler, this), m_DocumentManagerEventSubscriber);

  SUPER::InitializeAfterLoadingAndSaving();

  // Game object document subscribed to the true document originally but want to reroute it to the mock data so that is picks up the layer content on its own.
  // Therefore we need to unsubscribe the original subscriptions and replace them with ones to the mock data.
  UnsubscribeGameObjectEventHandlers();

  // These preserve the real scene document.
  m_pSceneObjectManager          = std::move(m_pObjectManager);
  m_pSceneCommandHistory         = std::move(m_pCommandHistory);
  m_pSceneSelectionManager       = std::move(m_pSelectionManager);
  m_pSceneObjectAccessor         = std::move(m_pObjectAccessor);
  m_pSceneDocumentObjectMetaData = std::move(m_DocumentObjectMetaData);
  m_pSceneGameObjectMetaData     = std::move(m_GameObjectMetaData);

  // Replace real scene elements with copies.
  m_pObjectManager = XII_DEFAULT_NEW(xiiSceneObjectManager);
  m_pObjectManager->SetDocument(this);
  m_pObjectManager->SwapStorage(m_pSceneObjectManager->GetStorage());
  m_pCommandHistory = XII_DEFAULT_NEW(xiiCommandHistory, this);
  m_pCommandHistory->SwapStorage(m_pSceneCommandHistory->GetStorage());
  m_pSelectionManager = XII_DEFAULT_NEW(xiiSelectionManager, m_pSceneObjectManager.Borrow());
  m_pSelectionManager->SwapStorage(m_pSceneSelectionManager->GetStorage());
  m_pObjectAccessor        = XII_DEFAULT_NEW(xiiObjectCommandAccessor, m_pCommandHistory.Borrow());
  using ObjectMetaData     = xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>;
  m_DocumentObjectMetaData = XII_DEFAULT_NEW(ObjectMetaData);
  m_DocumentObjectMetaData->SwapStorage(m_pSceneDocumentObjectMetaData->GetStorage());
  using GameObjectMetaData = xiiObjectMetaData<xiiUuid, xiiGameObjectMetaData>;
  m_GameObjectMetaData     = XII_DEFAULT_NEW(GameObjectMetaData);
  m_GameObjectMetaData->SwapStorage(m_pSceneGameObjectMetaData->GetStorage());

  // See comment above for UnsubscribeGameObjectEventHandlers.
  SubscribeGameObjectEventHandlers();

  UpdateLayers();
  if (const xiiDocumentObject* pLayerObject = GetLayerObject(GetActiveLayer()))
  {
    m_pLayerSelection->SetSelection(pLayerObject);
  }
}

const xiiDocumentObject* xiiScene2Document::GetSettingsObject() const
{
  /// This function is overwritten so that after redirecting to the active document this still accesses the original content and is not redirected.
  if (m_pSceneObjectManager == nullptr)
    return SUPER::GetSettingsObject();

  auto       pRoot = GetSceneObjectManager()->GetRootObject();
  xiiVariant value;
  XII_VERIFY(GetSceneObjectAccessor()->GetValue(pRoot, "Settings", value).Succeeded(), "The scene doc root should have a settings property.");
  xiiUuid id = value.Get<xiiUuid>();
  return GetSceneObjectManager()->GetObject(id);
}

void xiiScene2Document::HandleEngineMessage(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (const xiiPushObjectStateMsgToEditor* msg = xiiDynamicCast<const xiiPushObjectStateMsgToEditor*>(pMsg))
  {
    HandleObjectStateFromEngineMsg2(msg);
    return;
  }

  SUPER::HandleEngineMessage(pMsg);
}

xiiTaskGroupID xiiScene2Document::InternalSaveDocument(AfterSaveCallback callback)
{
  // We need to switch the active layer back to the original content as otherwise the scene will not save itself but instead the active layer's content into itself.
  SetActiveLayer(GetGuid()).LogFailure();
  return SUPER::InternalSaveDocument(callback);
}

void xiiScene2Document::SendGameWorldToEngine()
{
  SUPER::SendGameWorldToEngine();
  for (auto layer : m_Layers)
  {
    xiiSceneDocument* pLayer = layer.Value().m_pLayer;
    if (pLayer != this && pLayer != nullptr)
    {
      pLayer->SendDocumentOpenMessage(true);
    }
  }
}

void xiiScene2Document::LayerSelectionEventHandler(const xiiSelectionManagerEvent& e)
{
  const xiiDocumentObject* pObject = m_pLayerSelection->GetCurrentObject();
  // We can't change the active layer while a transaction is in progress at it will swap out the data storage the transaction is currently modifying.
  if (pObject && !m_pCommandHistory->IsInTransaction() && !m_pSceneCommandHistory->IsInTransaction())
  {
    if (pObject->GetType()->IsDerivedFrom(xiiGetStaticRTTI<xiiSceneLayer>()))
    {
      xiiUuid layerGuid = GetSceneObjectAccessor()->Get<xiiUuid>(pObject, "Layer");
      if (IsLayerLoaded(layerGuid))
      {
        SetActiveLayer(layerGuid).LogFailure();
      }
    }
  }
}

void xiiScene2Document::StructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
}

void xiiScene2Document::CommandHistoryEventHandler(const xiiCommandHistoryEvent& e)
{
  switch (e.m_Type)
  {
    case xiiCommandHistoryEvent::Type::UndoEnded:
    case xiiCommandHistoryEvent::Type::RedoEnded:
    case xiiCommandHistoryEvent::Type::TransactionEnded:
      UpdateLayers();
      break;
    default:
      return;
  }
}

void xiiScene2Document::DocumentManagerEventHandler(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentOpened:
    {
      if (xiiLayerDocument* pLayer = xiiDynamicCast<xiiLayerDocument*>(e.m_pDocument))
      {
        if (pLayer->GetMainDocument() != this)
          return;

        xiiUuid    layerGuid = e.m_pDocument->GetGuid();
        LayerInfo* pInfo     = nullptr;
        // Either the layer is currently being creating, in which case m_Layers can't be filled yet,
        // or an existing layer's state is toggled either internally by the scene or externally by the editor in which case the layer is known and we must react to it.
        if (m_Layers.TryGetValue(layerGuid, pInfo) && pInfo->m_pLayer != pLayer)
        {
          pInfo->m_pLayer = pLayer;

          xiiScene2LayerEvent e;
          e.m_Type      = xiiScene2LayerEvent::Type::LayerLoaded;
          e.m_layerGuid = layerGuid;
          m_LayerEvents.Broadcast(e);
        }
      }
    }
    break;
    case xiiDocumentManager::Event::Type::DocumentClosing:
    {
      if (e.m_pDocument->GetDynamicRTTI()->IsDerivedFrom<xiiLayerDocument>())
      {
        xiiUuid    layerGuid = e.m_pDocument->GetGuid();
        LayerInfo* pInfo     = nullptr;
        if (m_Layers.TryGetValue(layerGuid, pInfo))
        {
          pInfo->m_pLayer = nullptr;

          xiiScene2LayerEvent e;
          e.m_Type      = xiiScene2LayerEvent::Type::LayerUnloaded;
          e.m_layerGuid = layerGuid;
          m_LayerEvents.Broadcast(e);
        }
      }
    }
    break;
    default:
      break;
  }
}

void xiiScene2Document::HandleObjectStateFromEngineMsg2(const xiiPushObjectStateMsgToEditor* pMsg)
{
  xiiMap<xiiUuid, xiiHybridArray<const xiiPushObjectStateData*, 1>> layerToChanges;
  for (const xiiPushObjectStateData& change : pMsg->m_ObjectStates)
  {
    layerToChanges[change.m_LayerGuid].PushBack(&change);
  }

  const xiiUuid activeLayer = m_ActiveLayerGuid;
  for (auto it : layerToChanges)
  {
    if (SetActiveLayer(it.Key()).Failed())
      continue;

    auto pHistory = GetCommandHistory();

    pHistory->StartTransaction("Pull Object State");

    for (const xiiPushObjectStateData* pState : it.Value())
    {
      auto pObject = GetObjectManager()->GetObject(pState->m_ObjectGuid);

      if (!pObject)
        continue;

      // set the general transform of the object
      SetGlobalTransform(pObject, xiiTransform(pState->m_vPosition, pState->m_qRotation), TransformationChanges::Translation | TransformationChanges::Rotation);

      // if we also have bone transforms, attempt to set them as well
      if (pState->m_BoneTransforms.IsEmpty())
        continue;

      auto pAccessor = GetObjectAccessor();

      // check all components
      for (auto pComponent : pObject->GetChildren())
      {
        auto pComponentType = pComponent->GetType();

        const auto* pBoneManipAttr = pComponentType->GetAttributeByType<xiiBoneManipulatorAttribute>();

        // we can only apply bone transforms on components that have the xiiBoneManipulatorAttribute attribute
        if (pBoneManipAttr == nullptr)
          continue;

        auto pBonesProperty = pComponentType->FindPropertyByName(pBoneManipAttr->GetTransformProperty());
        XII_ASSERT_DEBUG(pBonesProperty, "Invalid transform property set on xiiBoneManipulatorAttribute");

        const xiiExposedParametersAttribute* pExposedParamsAttr = pBonesProperty->GetAttributeByType<xiiExposedParametersAttribute>();
        XII_ASSERT_DEBUG(pExposedParamsAttr, "Expected exposed parameters on xiiBoneManipulatorAttribute property");

        const xiiAbstractProperty* pParameterSourceProp = pComponentType->FindPropertyByName(pExposedParamsAttr->GetParametersSource());
        XII_ASSERT_DEBUG(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pExposedParamsAttr->GetParametersSource(), pComponentType->GetTypeName());

        // retrieve all the bone keys and values, these will contain the exposed default values, in case a bone has never been overridden before
        xiiVariantArray                    boneValues, boneKeys;
        xiiExposedParameterCommandAccessor proxy(pAccessor, pBonesProperty, pParameterSourceProp);
        proxy.GetValues(pComponent, pBonesProperty, boneValues);
        proxy.GetKeys(pComponent, pBonesProperty, boneKeys);

        // apply all the new bone transforms
        for (const auto& bone : pState->m_BoneTransforms)
        {
          // ignore bones that are unknown (not exposed somehow)
          xiiUInt32 idx = boneKeys.IndexOf(bone.Key());
          if (idx == xiiInvalidIndex)
            continue;

          XII_ASSERT_DEBUG(boneValues[idx].GetReflectedType() == xiiGetStaticRTTI<xiiExposedBone>(), "Expected an xiiExposedBone in variant");

          // retrieve the default/previous value of the bone
          const xiiExposedBone* pDefVal = reinterpret_cast<const xiiExposedBone*>(boneValues[idx].GetData());

          xiiExposedBone b;
          b.m_sName     = pDefVal->m_sName;   // same as the key
          b.m_sParent   = pDefVal->m_sParent; // this is what we don't have and therefore needed to retrieve the default values
          b.m_Transform = bone.Value();

          xiiVariant var;
          var.CopyTypedObject(&b, xiiGetStaticRTTI<xiiExposedBone>());

          proxy.SetValue(pComponent, pBonesProperty, var, bone.Key());
        }

        // found a component/property to apply bones to, so we can stop
        break;
      }
    }

    pHistory->FinishTransaction();
  }
  SetActiveLayer(activeLayer);
}

void xiiScene2Document::UpdateLayers()
{
  xiiSet<xiiUuid> layersBefore;
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    layersBefore.Insert(it.Key());
  }
  xiiSet<xiiUuid>                 layersAfter;
  xiiMap<xiiUuid, xiiUuid>        LayerToSceneObject;
  const xiiSceneDocumentSettings* pSettings = GetSettings<xiiSceneDocumentSettings>();
  for (const xiiSceneLayerBase* pLayerBase : pSettings->m_Layers)
  {
    if (const xiiSceneLayer* pLayer = xiiDynamicCast<const xiiSceneLayer*>(pLayerBase))
    {
      layersAfter.Insert(pLayer->m_Layer);
      xiiUuid objectGuid = m_Context.GetObjectGUID(xiiGetStaticRTTI<xiiSceneLayer>(), pLayer);
      LayerToSceneObject.Insert(pLayer->m_Layer, objectGuid);
    }
  }

  xiiSet<xiiUuid> layersRemoved = layersBefore;
  layersRemoved.Difference(layersAfter);
  for (auto it = layersRemoved.GetIterator(); it.IsValid(); ++it)
  {
    LayerRemoved(it.Key());
  }

  xiiSet<xiiUuid> layersAdded = layersAfter;
  layersAdded.Difference(layersBefore);
  for (auto it = layersAdded.GetIterator(); it.IsValid(); ++it)
  {
    LayerAdded(it.Key(), LayerToSceneObject[it.Key()]);
  }
}

void xiiScene2Document::SendLayerVisibility()
{
  xiiLayerVisibilityChangedMsgToEngine msg;
  for (auto& layer : m_Layers)
  {
    if (!layer.Value().m_bVisible)
    {
      // We are sending the hidden state because the default state is visible so we have less to send and less often.
      msg.m_HiddenLayers.PushBack(layer.Key());
    }
  }
  SendMessageToEngine(&msg);
}

void xiiScene2Document::LayerAdded(const xiiUuid& layerGuid, const xiiUuid& layerObjectGuid)
{
  LayerInfo info;
  info.m_pLayer     = nullptr;
  info.m_bVisible   = true;
  info.m_objectGuid = layerObjectGuid;
  m_Layers.Insert(layerGuid, info);

  xiiScene2LayerEvent e;
  e.m_Type      = xiiScene2LayerEvent::Type::LayerAdded;
  e.m_layerGuid = layerGuid;
  m_LayerEvents.Broadcast(e);

  //#TODO Decide whether to load a layer or not (persist as meta data? / user preferences?)
  SetLayerLoaded(layerGuid, true).LogFailure();
}

void xiiScene2Document::LayerRemoved(const xiiUuid& layerGuid)
{
  // Make sure removed layer is not active
  if (m_ActiveLayerGuid == layerGuid)
  {
    SetActiveLayer(GetGuid()).LogFailure();
  }

  SetLayerLoaded(layerGuid, false).LogFailure();

  xiiScene2LayerEvent e;
  e.m_Type      = xiiScene2LayerEvent::Type::LayerRemoved;
  e.m_layerGuid = layerGuid;
  m_LayerEvents.Broadcast(e);

  m_Layers.Remove(layerGuid);
}

xiiStatus xiiScene2Document::CreateLayer(const char* szName, xiiUuid& out_layerGuid)
{
  // We need to be the active layer in order to make changes to the layers.
  xiiStatus res = SetActiveLayer(GetGuid());
  if (res.Failed())
    return res;

  const xiiDocumentTypeDescriptor* pLayerDesc = xiiDocumentManager::GetDescriptorForDocumentType("Layer");

  xiiStringBuilder targetDirectory = GetDocumentPath();
  targetDirectory.RemoveFileExtension();
  targetDirectory.Append("_data");
  targetDirectory.AppendPath(szName);
  targetDirectory.Append(".", pLayerDesc->m_sFileExtension.GetData());

  xiiSceneDocument* pLayerDoc = nullptr;
  if (xiiOSFile::ExistsFile(targetDirectory))
  {
    xiiDocumentObject* pRoot = m_pSceneObjectManager->GetRootObject();
    pLayerDoc                = xiiDynamicCast<xiiSceneDocument*>(xiiQtEditorApp::GetSingleton()->OpenDocument(targetDirectory, xiiDocumentFlags::None, pRoot));

    if (m_Layers.Contains(pLayerDoc->GetGuid()))
    {
      return xiiStatus(xiiFmt("A layer named '{}' already exists in this scene.", szName));
    }
  }
  else
  {
    xiiDocumentObject* pRoot = m_pSceneObjectManager->GetRootObject();
    pLayerDoc                = xiiDynamicCast<xiiSceneDocument*>(xiiQtEditorApp::GetSingleton()->CreateDocument(targetDirectory, xiiDocumentFlags::None, pRoot));
    if (!pLayerDoc)
    {
      return xiiStatus(xiiFmt("Failed to create new layer '{0}'", targetDirectory));
    }
  }

  xiiObjectAccessorBase* pAccessor = GetSceneObjectAccessor();
  xiiStringBuilder       sTransactionText;
  pAccessor->StartTransaction(xiiFmt("Add Layer - '{}'", szName).GetText(sTransactionText));
  {
    auto     pRoot   = m_pSceneObjectManager->GetObject(GetSettingsObject()->GetGuid());
    xiiInt32 uiCount = 0;
    XII_VERIFY(pAccessor->GetCount(pRoot, "Layers", uiCount).Succeeded(), "Failed to get layer count.");
    xiiUuid sceneLayerGuid;
    XII_VERIFY(pAccessor->AddObject(pRoot, "Layers", uiCount, xiiGetStaticRTTI<xiiSceneLayer>(), sceneLayerGuid).Succeeded(), "Failed to add layer to scene.");
    auto pLayer = pAccessor->GetObject(sceneLayerGuid);
    XII_VERIFY(pAccessor->SetValue(pLayer, "Layer", pLayerDoc->GetGuid()).Succeeded(), "Failed to set layer GUID.");
  }
  pAccessor->FinishTransaction();

  LayerInfo* pInfo = nullptr;
  XII_ASSERT_DEV(m_Layers.Contains(pLayerDoc->GetGuid()), "FinishTransaction should have triggered UpdateLayers and filled m_Layers.");
  // We need to manually emit this here as when the layer doc was loaded DocumentManagerEventHandler will not fire as the document was not added as a layer yet.
  if (m_Layers.TryGetValue(pLayerDoc->GetGuid(), pInfo) && pInfo->m_pLayer != pLayerDoc)
  {
    pInfo->m_pLayer = pLayerDoc;

    xiiScene2LayerEvent e;
    e.m_Type      = xiiScene2LayerEvent::Type::LayerLoaded;
    e.m_layerGuid = pLayerDoc->GetGuid();
    m_LayerEvents.Broadcast(e);
  }
  out_layerGuid = pLayerDoc->GetGuid();
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiScene2Document::DeleteLayer(const xiiUuid& layerGuid)
{
  // We need to be the active layer in order to make changes to the layers.
  xiiStatus res = SetActiveLayer(GetGuid());
  if (res.Failed())
    return res;

  LayerInfo* pInfo = nullptr;
  if (!m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return xiiStatus("Unknown layer guid. Layer can't be deleted.");
  }

  if (!pInfo->m_objectGuid.IsValid())
  {
    return xiiStatus("Layer object guid not set, layer object unknown.");
  }

  const xiiDocumentObject* pObject = GetSceneObjectManager()->GetObject(pInfo->m_objectGuid);
  if (!pObject)
  {
    return xiiStatus("Layer object no longer valid.");
  }

  xiiStringBuilder sName("<Unknown>");
  {
    auto assetInfo = xiiAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
    if (assetInfo.isValid())
    {
      sName = xiiPathUtils::GetFileName(assetInfo->m_pAssetInfo->m_sDataDirParentRelativePath);
    }
    else
    {
      return xiiStatus("Could not resolve layer in xiiAssetCurator.");
    }
  }

  xiiObjectAccessorBase* pAccessor = GetSceneObjectAccessor();
  xiiStringBuilder       sTransactionText;
  pAccessor->StartTransaction(xiiFmt("Remove Layer - '{}'", sName).GetText(sTransactionText));
  {
    XII_VERIFY(pAccessor->RemoveObject(pObject).Succeeded(), "Failed to remove Layer.");
  }
  pAccessor->FinishTransaction();
  return xiiStatus(XII_SUCCESS);
}

const xiiUuid& xiiScene2Document::GetActiveLayer() const
{
  return m_ActiveLayerGuid;
}

xiiStatus xiiScene2Document::SetActiveLayer(const xiiUuid& layerGuid)
{
  XII_ASSERT_DEV(!m_pCommandHistory->IsInTransaction(), "Active layer must not be changed while an operation is in progress.");
  XII_ASSERT_DEV(!m_pSceneCommandHistory || !m_pSceneCommandHistory->IsInTransaction(), "Active layer must not be changed while an operation is in progress.");

  if (layerGuid == m_ActiveLayerGuid)
    return xiiStatus(XII_SUCCESS);

  if (layerGuid == GetGuid())
  {
    xiiDocumentObjectStructureEvent e;
    e.m_pDocument = this;
    e.m_EventType = xiiDocumentObjectStructureEvent::Type::BeforeReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);

    m_pObjectManager->SwapStorage(m_pSceneObjectManager->GetStorage());
    m_pCommandHistory->SwapStorage(m_pSceneCommandHistory->GetStorage());
    m_pSelectionManager->SwapStorage(m_pSceneSelectionManager->GetStorage());
    m_DocumentObjectMetaData->SwapStorage(m_pSceneDocumentObjectMetaData->GetStorage());
    m_GameObjectMetaData->SwapStorage(m_pSceneGameObjectMetaData->GetStorage());
    // m_pSceneObjectAccessor does not need to be modified

    e.m_EventType = xiiDocumentObjectStructureEvent::Type::AfterReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);
  }
  else
  {
    xiiDocument* pDoc = xiiDocumentManager::GetDocumentByGuid(layerGuid);
    if (!pDoc)
      return xiiStatus("Unloaded layer can't be made active.");

    xiiDocumentObjectStructureEvent e;
    e.m_pDocument = this;
    e.m_EventType = xiiDocumentObjectStructureEvent::Type::BeforeReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);

    m_pObjectManager->SwapStorage(pDoc->GetObjectManager()->GetStorage());
    m_pCommandHistory->SwapStorage(pDoc->GetCommandHistory()->GetStorage());
    m_pSelectionManager->SwapStorage(pDoc->GetSelectionManager()->GetStorage());
    m_DocumentObjectMetaData->SwapStorage(pDoc->m_DocumentObjectMetaData->GetStorage());
    // m_pSceneObjectAccessor does not need to be modified

    e.m_EventType = xiiDocumentObjectStructureEvent::Type::AfterReset;
    m_pObjectManager->m_StructureEvents.Broadcast(e);
  }

  const bool bVisualizers = xiiVisualizerManager::GetSingleton()->GetVisualizersActive(GetLayerDocument(m_ActiveLayerGuid));

  xiiVisualizerManager::GetSingleton()->SetVisualizersActive(GetLayerDocument(m_ActiveLayerGuid), false);

  {
    xiiSelectionManagerEvent se;
    se.m_pDocument = this;
    se.m_pObject   = nullptr;
    se.m_Type      = xiiSelectionManagerEvent::Type::SelectionSet;
    m_pSelectionManager->GetStorage()->m_Events.Broadcast(se);
  }
  {
    xiiCommandHistoryEvent ce;
    ce.m_pDocument = this;
    ce.m_Type      = xiiCommandHistoryEvent::Type::HistoryChanged;
    m_pCommandHistory->GetStorage()->m_Events.Broadcast(ce);
  }

  m_ActiveLayerGuid    = layerGuid;
  m_pActiveSubDocument = GetLayerDocument(layerGuid);
  {
    xiiScene2LayerEvent e;
    e.m_Type      = xiiScene2LayerEvent::Type::ActiveLayerChanged;
    e.m_layerGuid = layerGuid;
    m_LayerEvents.Broadcast(e);
  }
  {
    xiiDocumentEvent e;
    e.m_pDocument = this;
    e.m_Type      = xiiDocumentEvent::Type::ModifiedChanged;

    m_EventsOne.Broadcast(e);
    s_EventsAny.Broadcast(e);
  }
  {
    xiiActiveLayerChangedMsgToEngine msg;
    msg.m_ActiveLayer = layerGuid;
    SendMessageToEngine(&msg);
  }

  xiiVisualizerManager::GetSingleton()->SetVisualizersActive(GetLayerDocument(m_ActiveLayerGuid), bVisualizers);

  // Set selection to object that contains the active layer
  if (const xiiDocumentObject* pLayerObject = GetLayerObject(layerGuid))
  {
    m_pLayerSelection->SetSelection(pLayerObject);
  }
  return xiiStatus(XII_SUCCESS);
}

bool xiiScene2Document::IsLayerLoaded(const xiiUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return pInfo->m_pLayer != nullptr;
  }
  return false;
}

xiiStatus xiiScene2Document::SetLayerLoaded(const xiiUuid& layerGuid, bool bLoaded)
{
  if (GetGameMode() != GameMode::Enum::Off)
    return xiiStatus("Simulation must be stopped to change a layer's loaded state.");

  if (layerGuid == GetGuid() && !bLoaded)
    return xiiStatus("Cannot unload the scene itself.");

  // We can't unload the active layer
  if (!bLoaded && m_ActiveLayerGuid == layerGuid)
  {
    xiiStatus res = SetActiveLayer(GetGuid());
    if (res.Failed())
      return res;
  }

  LayerInfo* pInfo = nullptr;
  if (!m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return xiiStatus("Unknown layer guid. Layer can't be loaded / unloaded.");
  }

  if ((pInfo->m_pLayer != nullptr) == bLoaded)
    return xiiStatus(XII_SUCCESS);

  if (bLoaded)
  {
    xiiStringBuilder sAbsPath;
    if (layerGuid == GetGuid())
    {
      sAbsPath = GetDocumentPath();
    }
    else
    {
      auto assetInfo = xiiAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
      if (assetInfo.isValid())
      {
        sAbsPath = assetInfo->m_pAssetInfo->m_sAbsolutePath;
      }
      else
      {
        return xiiStatus("Could not resolve layer in xiiAssetCurator.");
      }
    }

    xiiDocument* pDoc = nullptr;
    // Pass our root into it to indicate what the parent context of the layer is.
    xiiDocumentObject* pRoot = m_pSceneObjectManager->GetRootObject();
    if (xiiDocument* pLayer = xiiQtEditorApp::GetSingleton()->OpenDocument(sAbsPath, xiiDocumentFlags::None, pRoot))
    {
      if (layerGuid != GetGuid() && pLayer->GetMainDocument() != this)
      {
        return xiiStatus("Layer already open in another window.");
      }

      // In case we are responding to e.g. an redo 'Add Layer' the layer is already loaded in the editor but we still want to enforce that the event is fired every time after adding a layer.
      if (pInfo->m_pLayer != pLayer)
      {
        pInfo->m_pLayer = xiiDynamicCast<xiiSceneDocument*>(pLayer);

        xiiScene2LayerEvent e;
        e.m_Type      = xiiScene2LayerEvent::Type::LayerLoaded;
        e.m_layerGuid = layerGuid;
        m_LayerEvents.Broadcast(e);
      }

      return xiiStatus(XII_SUCCESS);
    }
    else
    {
      return xiiStatus("Could not load layer, see log for more information.");
    }
  }
  else
  {
    if (pInfo->m_pLayer == nullptr)
      return xiiStatus(XII_SUCCESS);

    // Unload document (save and close)
    xiiDocumentManager* pManager = pInfo->m_pLayer->GetDocumentManager();
    pManager->CloseDocument(pInfo->m_pLayer);
    pInfo->m_pLayer = nullptr;

    // xiiScene2LayerEvent e;
    // e.m_Type = xiiScene2LayerEvent::Type::LayerUnloaded;
    // e.m_layerGuid = layerGuid;
    // m_LayerEvents.Broadcast(e);

    return xiiStatus(XII_SUCCESS);
  }
}

void xiiScene2Document::GetAllLayers(xiiDynamicArray<xiiUuid>& out_LayerGuids)
{
  out_LayerGuids.Clear();
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    out_LayerGuids.PushBack(it.Key());
  }
}

void xiiScene2Document::GetLoadedLayers(xiiDynamicArray<xiiSceneDocument*>& out_Layers) const
{
  out_Layers.Clear();
  for (auto it = m_Layers.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_pLayer)
    {
      out_Layers.PushBack(it.Value().m_pLayer);
    }
  }
}

bool xiiScene2Document::IsLayerVisible(const xiiUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return pInfo->m_bVisible;
  }
  return false;
}

xiiStatus xiiScene2Document::SetLayerVisible(const xiiUuid& layerGuid, bool bVisible)
{
  LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    if (pInfo->m_bVisible != bVisible)
    {
      pInfo->m_bVisible = bVisible;
      {
        xiiScene2LayerEvent e;
        e.m_Type      = bVisible ? xiiScene2LayerEvent::Type::LayerVisible : xiiScene2LayerEvent::Type::LayerInvisible;
        e.m_layerGuid = layerGuid;
        m_LayerEvents.Broadcast(e);
      }
      SendLayerVisibility();
    }
    return xiiStatus(XII_SUCCESS);
  }
  return xiiStatus("Unknown layer.");
}

const xiiDocumentObject* xiiScene2Document::GetLayerObject(const xiiUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return GetSceneObjectManager()->GetObject(pInfo->m_objectGuid);
  }
  return nullptr;
}

xiiSceneDocument* xiiScene2Document::GetLayerDocument(const xiiUuid& layerGuid) const
{
  const LayerInfo* pInfo = nullptr;
  if (m_Layers.TryGetValue(layerGuid, pInfo))
  {
    return pInfo->m_pLayer;
  }
  return nullptr;
}

bool xiiScene2Document::IsAnyLayerModified() const
{
  for (auto& layer : m_Layers)
  {
    auto pLayer = layer.Value().m_pLayer;
    if (pLayer && pLayer->IsModified())
      return true;
  }

  return false;
}
