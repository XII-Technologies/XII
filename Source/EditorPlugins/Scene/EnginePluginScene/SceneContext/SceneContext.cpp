#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <EnginePluginScene/SceneView/SceneView.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Interfaces/SoundInterface.h>
#include <Core/Prefabs/PrefabResource.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginScene/SceneContext/LayerContext.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/VisualScript/VisualScriptComponent.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneContext, 1, xiiRTTIDefaultAllocator<xiiSceneContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Scene;Prefab;PropertyAnim"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiSceneContext::ComputeHierarchyBounds(xiiGameObject* pObj, xiiBoundingBoxSphere& bounds)
{
  pObj->UpdateGlobalTransformAndBounds();
  const auto& b = pObj->GetGlobalBounds();

  if (b.IsValid())
    bounds.ExpandToInclude(b);

  for (auto it = pObj->GetChildren(); it.IsValid(); ++it)
  {
    ComputeHierarchyBounds(it, bounds);
  }
}

void xiiSceneContext::DrawSelectionBounds(const xiiViewHandle& hView)
{
  if (!m_bRenderSelectionBoxes)
    return;

  XII_LOCK(m_pWorld->GetWriteMarker());

  for (const auto& obj : m_Selection)
  {
    xiiBoundingBoxSphere bounds;
    bounds.SetInvalid();

    xiiGameObject* pObj;
    if (!m_pWorld->TryGetObject(obj, pObj))
      continue;

    ComputeHierarchyBounds(pObj, bounds);

    if (bounds.IsValid())
    {
      xiiDebugRenderer::DrawLineBoxCorners(hView, bounds.GetBox(), 0.25f, xiiColorScheme::LightUI(xiiColorScheme::Yellow));
    }
  }
}

void xiiSceneContext::UpdateInvisibleLayerTags()
{
  if (m_bInvisibleLayersDirty)
  {
    m_bInvisibleLayersDirty = false;

    xiiMap<xiiUuid, xiiUInt32> layerGuidToIndex;
    for (xiiUInt32 i = 0; i < m_Layers.GetCount(); i++)
    {
      if (m_Layers[i] != nullptr)
      {
        layerGuidToIndex.Insert(m_Layers[i]->GetDocumentGuid(), i);
      }
    }

    xiiHybridArray<xiiTag, 1> newInvisibleLayerTags;
    newInvisibleLayerTags.Reserve(m_InvisibleLayers.GetCount());
    for (const xiiUuid& guid : m_InvisibleLayers)
    {
      xiiUInt32 uiLayerID = 0;
      if (layerGuidToIndex.TryGetValue(guid, uiLayerID))
      {
        newInvisibleLayerTags.PushBack(m_Layers[uiLayerID]->GetLayerTag());
      }
      else if (guid == GetDocumentGuid())
      {
        newInvisibleLayerTags.PushBack(m_LayerTag);
      }
    }

    for (xiiEngineProcessViewContext* pView : m_ViewContexts)
    {
      if (pView)
      {
        static_cast<xiiSceneViewContext*>(pView)->SetInvisibleLayerTags(m_InvisibleLayerTags.GetArrayPtr(), newInvisibleLayerTags.GetArrayPtr());
      }
    }
    m_InvisibleLayerTags.Swap(newInvisibleLayerTags);
  }
}

xiiSceneContext::xiiSceneContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
  m_bRenderSelectionOverlay = true;
  m_bRenderSelectionBoxes   = true;
  m_bRenderShapeIcons       = true;
  m_fGridDensity            = 0;
  m_GridTransform.SetIdentity();
  m_pWorld = nullptr;

  xiiVisualScriptComponent::GetActivityEvents().AddEventHandler(xiiMakeDelegate(&xiiSceneContext::OnVisualScriptActivity, this));
  xiiResourceManager::GetManagerEvents().AddEventHandler(xiiMakeDelegate(&xiiSceneContext::OnResourceManagerEvent, this));
  xiiGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(xiiMakeDelegate(&xiiSceneContext::GameApplicationEventHandler, this));
}

xiiSceneContext::~xiiSceneContext()
{
  xiiVisualScriptComponent::GetActivityEvents().RemoveEventHandler(xiiMakeDelegate(&xiiSceneContext::OnVisualScriptActivity, this));
  xiiResourceManager::GetManagerEvents().RemoveEventHandler(xiiMakeDelegate(&xiiSceneContext::OnResourceManagerEvent, this));
  xiiGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(xiiMakeDelegate(&xiiSceneContext::GameApplicationEventHandler, this));
}

void xiiSceneContext::HandleMessage(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiWorldSettingsMsgToEngine>())
  {
    // this message comes exactly once per 'update', afterwards there will be 1 to n redraw messages
    HandleWorldSettingsMsg(static_cast<const xiiWorldSettingsMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiSimulationSettingsMsgToEngine>())
  {
    HandleSimulationSettingsMsg(static_cast<const xiiSimulationSettingsMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiGridSettingsMsgToEngine>())
  {
    HandleGridSettingsMsg(static_cast<const xiiGridSettingsMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiObjectsForDebugVisMsgToEngine>())
  {
    HandleObjectsForDebugVisMsg(static_cast<const xiiObjectsForDebugVisMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiGameModeMsgToEngine>())
  {
    HandleGameModeMsg(static_cast<const xiiGameModeMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiObjectSelectionMsgToEngine>())
  {
    HandleSelectionMsg(static_cast<const xiiObjectSelectionMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiQuerySelectionBBoxMsgToEngine>())
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiExposedDocumentObjectPropertiesMsgToEngine>())
  {
    HandleExposedPropertiesMsg(static_cast<const xiiExposedDocumentObjectPropertiesMsgToEngine*>(pMsg));
    return;
  }

  if (const xiiExportSceneGeometryMsgToEngine* msg = xiiDynamicCast<const xiiExportSceneGeometryMsgToEngine*>(pMsg))
  {
    HandleSceneGeometryMsg(msg);
    return;
  }

  if (const xiiPullObjectStateMsgToEngine* msg = xiiDynamicCast<const xiiPullObjectStateMsgToEngine*>(pMsg))
  {
    HandlePullObjectStateMsg(msg);
    return;
  }

  if (pMsg->IsInstanceOf<xiiViewRedrawMsgToEngine>())
  {
    HandleViewRedrawMsg(static_cast<const xiiViewRedrawMsgToEngine*>(pMsg));
    // fall through
  }

  if (pMsg->IsInstanceOf<xiiActiveLayerChangedMsgToEngine>())
  {
    HandleActiveLayerChangedMsg(static_cast<const xiiActiveLayerChangedMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->IsInstanceOf<xiiObjectTagMsgToEngine>())
  {
    HandleTagMsgToEngineMsg(static_cast<const xiiObjectTagMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->IsInstanceOf<xiiLayerVisibilityChangedMsgToEngine>())
  {
    HandleLayerVisibilityChangedMsgToEngineMsg(static_cast<const xiiLayerVisibilityChangedMsgToEngine*>(pMsg));
    return;
  }

  xiiEngineProcessDocumentContext::HandleMessage(pMsg);

  if (pMsg->IsInstanceOf<xiiEntityMsgToEngine>())
  {
    XII_LOCK(m_pWorld->GetWriteMarker());
    AddLayerIndexTag(*static_cast<const xiiEntityMsgToEngine*>(pMsg), m_Context, m_LayerTag);
  }
}

void xiiSceneContext::HandleViewRedrawMsg(const xiiViewRedrawMsgToEngine* pMsg)
{
  if (m_bUpdateAllLocalBounds)
  {
    m_bUpdateAllLocalBounds = false;

    XII_LOCK(m_pWorld->GetWriteMarker());

    for (auto it = m_pWorld->GetObjects(); it.IsValid(); ++it)
    {
      it->UpdateLocalBounds();
    }
  }

  auto pDocView = GetViewContext(pMsg->m_uiViewID);
  if (pDocView)
    DrawSelectionBounds(pDocView->GetViewHandle());

  AnswerObjectStatePullRequest(pMsg);
  UpdateInvisibleLayerTags();
}

void xiiSceneContext::AnswerObjectStatePullRequest(const xiiViewRedrawMsgToEngine* pMsg)
{
  if (m_pWorld->GetWorldSimulationEnabled() || m_PushObjectStateMsg.m_ObjectStates.IsEmpty())
    return;

  XII_LOCK(m_pWorld->GetReadMarker());

  for (auto& state : m_PushObjectStateMsg.m_ObjectStates)
  {
    xiiWorldRttiConverterContext* pContext = GetContextForLayer(state.m_LayerGuid);
    if (!pContext)
      return;

    // if the handle map is currently empty, the scene has not yet been sent over
    // return and try again later
    if (pContext->m_GameObjectMap.GetHandleToGuidMap().IsEmpty())
      return;
  }

  // now we need to adjust the transforms for all objects that were not directly pulled
  // ie. nodes inside instantiated prefabs
  for (auto& state : m_PushObjectStateMsg.m_ObjectStates)
  {
    // ignore the ones that we accessed directly, their transform is correct already
    if (!state.m_bAdjustFromPrefabRootChild)
      continue;

    xiiWorldRttiConverterContext* pContext = GetContextForLayer(state.m_LayerGuid);
    if (!pContext)
      continue;

    const auto& objectMapper = pContext->m_GameObjectMap;

    xiiGameObjectHandle hObject = objectMapper.GetHandle(state.m_ObjectGuid);

    // if this object does not exist anymore, this is not considered a problem (user may have deleted it)
    xiiGameObject* pObject;
    if (!m_pWorld->TryGetObject(hObject, pObject))
      continue;

    // we expect the object to have a child, if none is there yet, we assume the prefab
    // instantiation has not happened yet
    // stop the whole process and try again later
    if (pObject->GetChildCount() == 0)
      return;

    const xiiGameObject* pChild = pObject->GetChildren();

    const xiiVec3 localPos = pChild->GetLocalPosition();
    const xiiQuat localRot = pChild->GetLocalRotation();

    // now adjust the position
    state.m_vPosition -= state.m_qRotation * -localRot * localPos;
    state.m_qRotation = state.m_qRotation * -localRot;
  }

  // send a return message with the result
  m_PushObjectStateMsg.m_DocumentGuid = pMsg->m_DocumentGuid;
  SendProcessMessage(&m_PushObjectStateMsg);

  m_PushObjectStateMsg.m_ObjectStates.Clear();
}

void xiiSceneContext::HandleActiveLayerChangedMsg(const xiiActiveLayerChangedMsgToEngine* pMsg)
{
  m_ActiveLayer = pMsg->m_ActiveLayer;
}

void xiiSceneContext::HandleTagMsgToEngineMsg(const xiiObjectTagMsgToEngine* pMsg)
{
  XII_LOCK(m_pWorld->GetWriteMarker());

  xiiGameObjectHandle hObject = GetActiveContext().m_GameObjectMap.GetHandle(pMsg->m_ObjectGuid);

  const xiiTag& tag = xiiTagRegistry::GetGlobalRegistry().RegisterTag(pMsg->m_sTag);

  xiiGameObject* pObject;
  if (m_pWorld->TryGetObject(hObject, pObject))
  {
    if (pMsg->m_bApplyOnAllChildren)
    {
      if (pMsg->m_bSetTag)
        SetTagRecursive(pObject, tag);
      else
        ClearTagRecursive(pObject, tag);
    }
    else
    {
      if (pMsg->m_bSetTag)
        pObject->SetTag(tag);
      else
        pObject->RemoveTag(tag);
    }
  }
}

void xiiSceneContext::HandleLayerVisibilityChangedMsgToEngineMsg(const xiiLayerVisibilityChangedMsgToEngine* pMsg)
{
  m_InvisibleLayers       = pMsg->m_HiddenLayers;
  m_bInvisibleLayersDirty = true;
}

void xiiSceneContext::HandleGridSettingsMsg(const xiiGridSettingsMsgToEngine* pMsg)
{
  m_fGridDensity = pMsg->m_fGridDensity;
  if (m_fGridDensity != 0.0f)
  {
    m_GridTransform.m_vPosition = pMsg->m_vGridCenter;

    if (pMsg->m_vGridTangent1.IsZero())
    {
      m_GridTransform.m_vScale.SetZero();
    }
    else
    {
      m_GridTransform.m_vScale.Set(1.0f);

      xiiMat3 mRot;
      mRot.SetColumn(0, pMsg->m_vGridTangent1);
      mRot.SetColumn(1, pMsg->m_vGridTangent2);
      mRot.SetColumn(2, pMsg->m_vGridTangent1.CrossRH(pMsg->m_vGridTangent2));
      m_GridTransform.m_qRotation.SetFromMat3(mRot);
    }
  }
}

void xiiSceneContext::HandleSimulationSettingsMsg(const xiiSimulationSettingsMsgToEngine* pMsg)
{
  const bool        bSimulate = pMsg->m_bSimulateWorld;
  xiiGameStateBase* pState    = GetGameState();
  m_pWorld->GetClock().SetSpeed(pMsg->m_fSimulationSpeed);

  if (pState == nullptr && bSimulate != m_pWorld->GetWorldSimulationEnabled())
  {
    m_pWorld->SetWorldSimulationEnabled(bSimulate);

    if (bSimulate)
      OnSimulationEnabled();
    else
      OnSimulationDisabled();
  }
}

void xiiSceneContext::HandleWorldSettingsMsg(const xiiWorldSettingsMsgToEngine* pMsg)
{
  m_bRenderSelectionOverlay = pMsg->m_bRenderOverlay;
  m_bRenderShapeIcons       = pMsg->m_bRenderShapeIcons;
  m_bRenderSelectionBoxes   = pMsg->m_bRenderSelectionBoxes;

  if (pMsg->m_bAddAmbientLight)
    AddAmbientLight(true, false);
  else
    RemoveAmbientLight();
}

void xiiSceneContext::QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (m_Selection.IsEmpty())
    return;

  xiiBoundingBoxSphere bounds;
  bounds.SetInvalid();

  {
    XII_LOCK(m_pWorld->GetWriteMarker());

    for (const auto& obj : m_Selection)
    {
      xiiGameObject* pObj;
      if (!m_pWorld->TryGetObject(obj, pObj))
        continue;

      ComputeHierarchyBounds(pObj, bounds);
    }

    // if there are no valid bounds, at all, use dummy bounds for each object
    if (!bounds.IsValid())
    {
      for (const auto& obj : m_Selection)
      {
        xiiGameObject* pObj;
        if (!m_pWorld->TryGetObject(obj, pObj))
          continue;

        bounds.ExpandToInclude(xiiBoundingBoxSphere(pObj->GetGlobalPosition(), xiiVec3(0.0f), 0.0f));
      }
    }
  }

  // XII_ASSERT_DEV(bounds.IsValid() && !bounds.IsNaN(), "Invalid bounds");

  if (!bounds.IsValid() || bounds.IsNaN())
  {
    xiiLog::Error("Selection has no valid bounding box");
    return;
  }

  const xiiQuerySelectionBBoxMsgToEngine* msg = static_cast<const xiiQuerySelectionBBoxMsgToEngine*>(pMsg);

  xiiQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID     = msg->m_uiViewID;
  res.m_iPurpose     = msg->m_iPurpose;
  res.m_vCenter      = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtends;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}

void xiiSceneContext::OnSimulationEnabled()
{
  xiiLog::Info("World Simulation enabled");

  xiiSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentGuid(), false);

  xiiResourceManager::ReloadAllResources(false);

  xiiGameApplication::GetGameApplicationInstance()->ReinitializeInputConfig();

  if (xiiSoundInterface* pSoundInterface = xiiSingletonRegistry::GetSingletonInstance<xiiSoundInterface>())
  {
    pSoundInterface->SetListenerOverrideMode(true);
  }
}

void xiiSceneContext::OnSimulationDisabled()
{
  xiiLog::Info("World Simulation disabled");

  xiiResourceManager::ResetAllResources();

  if (xiiSoundInterface* pSoundInterface = xiiSingletonRegistry::GetSingletonInstance<xiiSoundInterface>())
  {
    pSoundInterface->SetListenerOverrideMode(false);
  }
}

xiiGameStateBase* xiiSceneContext::GetGameState() const
{
  return xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameStateLinkedToWorld(m_pWorld);
}

xiiUInt32 xiiSceneContext::RegisterLayer(xiiLayerContext* pLayer)
{
  m_bInvisibleLayersDirty = true;
  m_Contexts.PushBack(&pLayer->m_Context);
  for (xiiUInt32 i = 0; i < m_Layers.GetCount(); ++i)
  {
    if (m_Layers[i] == nullptr)
    {
      m_Layers[i] = pLayer;
      return i;
    }
  }

  m_Layers.PushBack(pLayer);
  return m_Layers.GetCount() - 1;
}

void xiiSceneContext::UnregisterLayer(xiiLayerContext* pLayer)
{
  m_Contexts.RemoveAndSwap(&pLayer->m_Context);
  for (xiiUInt32 i = 0; i < m_Layers.GetCount(); ++i)
  {
    if (m_Layers[i] == pLayer)
    {
      m_Layers[i] = nullptr;
    }
  }

  while (!m_Layers.IsEmpty() && m_Layers.PeekBack() == nullptr)
    m_Layers.PopBack();
}

void xiiSceneContext::AddLayerIndexTag(const xiiEntityMsgToEngine& msg, xiiWorldRttiConverterContext& ref_context, const xiiTag& layerTag)
{
  if (msg.m_change.m_Change.m_Operation == xiiObjectChangeType::NodeAdded)
  {
    if ((msg.m_change.m_Change.m_sProperty == "Children" || msg.m_change.m_Change.m_sProperty.IsEmpty()) && msg.m_change.m_Change.m_Value.IsA<xiiUuid>())
    {
      const xiiUuid&         object = msg.m_change.m_Change.m_Value.Get<xiiUuid>();
      xiiRttiConverterObject target = ref_context.GetObjectByGUID(object);
      if (target.m_pType == xiiGetStaticRTTI<xiiGameObject>() && target.m_pObject != nullptr)
      {
        // We do postpone tagging until after the first frame so that prefab references are instantiated and affected as well.
        xiiGameObject* pObject = static_cast<xiiGameObject*>(target.m_pObject);
        m_ObjectsToTag.PushBack({pObject->GetHandle(), layerTag});
      }
    }
  }
}

const xiiArrayPtr<const xiiTag> xiiSceneContext::GetInvisibleLayerTags() const
{
  return m_InvisibleLayerTags.GetArrayPtr();
}

void xiiSceneContext::OnInitialize()
{
  XII_LOCK(m_pWorld->GetWriteMarker());
  if (!m_ActiveLayer.IsValid())
    m_ActiveLayer = m_DocumentGuid;
  m_Contexts.PushBack(&m_Context);

  m_LayerTag = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Layer_Scene");

  xiiShadowPool::AddExcludeTagToWhiteList(m_LayerTag);
}

void xiiSceneContext::OnDeinitialize()
{
  m_Selection.Clear();
  m_SelectionWithChildren.Clear();
  m_SelectionWithChildrenSet.Clear();
  m_hSkyLight.Invalidate();
  m_hDirectionalLight.Invalidate();
  m_LayerTag = xiiTag();
  for (xiiLayerContext* pLayer : m_Layers)
  {
    if (pLayer != nullptr)
      pLayer->SceneDeinitialized();
  }
}

xiiEngineProcessViewContext* xiiSceneContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiSceneViewContext, this);
}

void xiiSceneContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}

void xiiSceneContext::HandleSelectionMsg(const xiiObjectSelectionMsgToEngine* pMsg)
{
  m_Selection.Clear();
  m_SelectionWithChildrenSet.Clear();
  m_SelectionWithChildren.Clear();

  xiiStringBuilder sSel = pMsg->m_sSelection;
  xiiStringBuilder sGuid;

  auto pWorld = m_pWorld;
  XII_LOCK(pWorld->GetReadMarker());

  while (!sSel.IsEmpty())
  {
    sGuid.SetSubString_ElementCount(sSel.GetData() + 1, 40);
    sSel.Shrink(41, 0);

    const xiiUuid guid = xiiConversionUtils::ConvertStringToUuid(sGuid);

    auto hObject = GetActiveContext().m_GameObjectMap.GetHandle(guid);

    if (!hObject.IsInvalidated())
    {
      m_Selection.PushBack(hObject);

      xiiGameObject* pObject;
      if (pWorld->TryGetObject(hObject, pObject))
        InsertSelectedChildren(pObject);
    }
  }

  for (auto it = m_SelectionWithChildrenSet.GetIterator(); it.IsValid(); ++it)
  {
    m_SelectionWithChildren.PushBack(it.Key());
  }
}

void xiiSceneContext::OnPlayTheGameModeStarted(const xiiTransform* pStartPosition)
{
  if (xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState() != nullptr)
  {
    xiiLog::Warning("A Play-the-Game instance is already running, cannot launch a second in parallel.");
    return;
  }

  xiiLog::Info("Starting Play-the-Game mode");

  xiiSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentGuid(), false);

  xiiResourceManager::ReloadAllResources(false);

  m_pWorld->GetClock().SetSpeed(1.0f);
  m_pWorld->SetWorldSimulationEnabled(true);

  xiiGameApplication::GetGameApplicationInstance()->ReinitializeInputConfig();

  xiiGameApplicationBase::GetGameApplicationBaseInstance()->ActivateGameState(m_pWorld, pStartPosition).IgnoreResult();

  xiiGameModeMsgToEditor msgRet;
  msgRet.m_DocumentGuid = GetDocumentGuid();
  msgRet.m_bRunningPTG  = true;

  SendProcessMessage(&msgRet);

  if (xiiSoundInterface* pSoundInterface = xiiSingletonRegistry::GetSingletonInstance<xiiSoundInterface>())
  {
    pSoundInterface->SetListenerOverrideMode(false);
  }
}


void xiiSceneContext::OnVisualScriptActivity(const xiiVisualScriptComponentActivityEvent& e)
{
  // component handles are not unique across different worlds, in fact it is very likely that different worlds contain identical handles
  // therefore we first need to filter out, whether the component comes from the same world, as this context operates on
  if (e.m_pComponent->GetWorld() != GetWorld())
    return;

  XII_ASSERT_DEV(e.m_pComponent->GetDebugOutput(), "This component should not send debug data.");

  for (xiiWorldRttiConverterContext* pContext : GetAllContexts())
  {
    const xiiUuid guid = pContext->m_ComponentMap.GetGuid(e.m_pComponent->GetHandle());

    if (!guid.IsValid())
      return;

    xiiVisualScriptActivityMsgToEditor msg;
    msg.m_DocumentGuid  = GetDocumentGuid(); // #TODO: Should this be layer or scene guid?
    msg.m_ComponentGuid = guid;

    xiiMemoryStreamContainerWrapperStorage<xiiDataBuffer> storage(&msg.m_Activity);
    xiiMemoryStreamWriter                                 writer(&storage);

    writer << e.m_pActivity->m_ActiveExecutionConnections.GetCount();
    writer << e.m_pActivity->m_ActiveDataConnections.GetCount();

    for (const auto& con : e.m_pActivity->m_ActiveExecutionConnections)
    {
      writer << con;
    }

    for (const auto& con : e.m_pActivity->m_ActiveDataConnections)
    {
      writer << con;
    }

    SendProcessMessage(&msg);
  }
}

void xiiSceneContext::OnResourceManagerEvent(const xiiResourceManagerEvent& e)
{
  if (e.m_Type == xiiResourceManagerEvent::Type::ReloadAllResources)
  {
    // when resources get reloaded, make sure to update all object bounds
    // this is to prevent culling errors after meshes got transformed etc.
    m_bUpdateAllLocalBounds = true;
  }
}

void xiiSceneContext::GameApplicationEventHandler(const xiiGameApplicationExecutionEvent& e)
{
  if (e.m_Type == xiiGameApplicationExecutionEvent::Type::AfterUpdatePlugins && !m_ObjectsToTag.IsEmpty())
  {
    // At this point the world was ticked once and prefab instances are instantiated and will be affected by SetTagRecursive.
    XII_LOCK(m_pWorld->GetWriteMarker());
    for (const TagGameObject& tagObject : m_ObjectsToTag)
    {
      xiiGameObject* pObject = nullptr;
      if (m_pWorld->TryGetObject(tagObject.m_hObject, pObject))
      {
        SetTagRecursive(pObject, tagObject.m_Tag);
      }
    }
    m_ObjectsToTag.Clear();
  }
}

void xiiSceneContext::HandleObjectsForDebugVisMsg(const xiiObjectsForDebugVisMsgToEngine* pMsg)
{
  XII_LOCK(GetWorld()->GetWriteMarker());

  const xiiArrayPtr<const xiiUuid> guids(reinterpret_cast<const xiiUuid*>(pMsg->m_Objects.GetData()), pMsg->m_Objects.GetCount() / sizeof(xiiUuid));

  for (auto guid : guids)
  {
    auto hComp = GetActiveContext().m_ComponentMap.GetHandle(guid);

    if (hComp.IsInvalidated())
      continue;

    xiiEventMessageHandlerComponent* pComp = nullptr;
    if (!m_pWorld->TryGetComponent(hComp, pComp))
      continue;

    pComp->SetDebugOutput(true);
  }
}

void xiiSceneContext::HandleGameModeMsg(const xiiGameModeMsgToEngine* pMsg)
{
  xiiGameStateBase* pState = GetGameState();

  if (pMsg->m_bEnablePTG)
  {
    if (pState != nullptr)
    {
      xiiLog::Error("Cannot start Play-the-Game, there is already a game state active for this world");
      return;
    }

    if (pMsg->m_bUseStartPosition)
    {
      xiiQuat qRot;
      qRot.SetShortestRotation(xiiVec3(1, 0, 0), pMsg->m_vStartDirection);

      xiiTransform tStart(pMsg->m_vStartPosition, qRot);

      OnPlayTheGameModeStarted(&tStart);
    }
    else
    {
      OnPlayTheGameModeStarted(nullptr);
    }
  }
  else
  {
    if (pState == nullptr)
      return;

    xiiLog::Info("Attempting to stop Play-the-Game mode");
    pState->RequestQuit();
  }
}

void xiiSceneContext::InsertSelectedChildren(const xiiGameObject* pObject)
{
  m_SelectionWithChildrenSet.Insert(pObject->GetHandle());

  auto it = pObject->GetChildren();

  while (it.IsValid())
  {
    InsertSelectedChildren(it);

    it.Next();
  }
}

xiiStatus xiiSceneContext::ExportDocument(const xiiExportDocumentMsgToEngine* pMsg)
{
  if (!m_Context.m_UnknownTypes.IsEmpty())
  {
    xiiStringBuilder s;

    s.Append("Scene / prefab export failed: ");

    for (const xiiString& sType : m_Context.m_UnknownTypes)
    {
      s.AppendFormat("'{}' is unknown. ", sType);
    }

    return xiiStatus(s.GetView());
  }

  // make sure the world has been updated at least once, otherwise components aren't initialized
  // and messages for geometry extraction won't be delivered
  // this is necessary for the scene export modifiers to work
  {
    XII_LOCK(m_pWorld->GetWriteMarker());
    m_pWorld->SetWorldSimulationEnabled(false);
    m_pWorld->Update();
  }

  // #TODO layers
  xiiSceneExportModifier::ApplyAllModifiers(*m_pWorld, GetDocumentGuid(), true);

  xiiDeferredFileWriter file;
  file.SetOutput(pMsg->m_sOutputFile);

  // Export
  {
    // File Header
    {
      xiiAssetFileHeader header;
      header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
      header.Write(file).IgnoreResult();

      const char* szSceneTag = "[xiiBinaryScene]";
      file.WriteBytes(szSceneTag, sizeof(char) * 16).IgnoreResult();
    }

    const xiiTag& tagEditor               = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
    const xiiTag& tagNoExport             = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Exclude From Export");
    const xiiTag& tagEditorPrefabInstance = xiiTagRegistry::GetGlobalRegistry().RegisterTag("EditorPrefabInstance");

    xiiTagSet tags;
    tags.Set(tagEditor);
    tags.Set(tagEditorPrefabInstance);
    tags.Set(tagNoExport);

    xiiWorldWriter ww;
    ww.WriteWorld(file, *m_pWorld, &tags);

    ExportExposedParameters(ww, file);
  }

  // do the actual file writing
  if (file.Close().Failed())
    return xiiStatus(xiiFmt("Writing to '{}' failed.", pMsg->m_sOutputFile));

  return xiiStatus(XII_SUCCESS);
}

void xiiSceneContext::ExportExposedParameters(const xiiWorldWriter& ww, xiiDeferredFileWriter& file) const
{
  xiiHybridArray<xiiExposedPrefabParameterDesc, 16> exposedParams;

  for (const auto& esp : m_ExposedSceneProperties)
  {
    xiiGameObject* pTargetObject = nullptr;
    const xiiRTTI* pComponenType = nullptr;

    xiiRttiConverterObject obj = m_Context.GetObjectByGUID(esp.m_Object);

    if (obj.m_pType == nullptr)
      continue;

    if (obj.m_pType->IsDerivedFrom<xiiGameObject>())
    {
      pTargetObject = reinterpret_cast<xiiGameObject*>(obj.m_pObject);
    }
    else if (obj.m_pType->IsDerivedFrom<xiiComponent>())
    {
      xiiComponent* pComponent = reinterpret_cast<xiiComponent*>(obj.m_pObject);

      pTargetObject = pComponent->GetOwner();
      pComponenType = obj.m_pType;
    }

    if (pTargetObject == nullptr)
      continue;

    xiiInt32 iFoundObjRoot  = -1;
    xiiInt32 iFoundObjChild = -1;

    // search for the target object in the exported objects
    {
      const auto& objects = ww.GetAllWrittenRootObjects();
      for (xiiUInt32 i = 0; i < objects.GetCount(); ++i)
      {
        if (objects[i] == pTargetObject)
        {
          iFoundObjRoot = i;
          break;
        }
      }

      if (iFoundObjRoot < 0)
      {
        const auto& objects = ww.GetAllWrittenChildObjects();
        for (xiiUInt32 i = 0; i < objects.GetCount(); ++i)
        {
          if (objects[i] == pTargetObject)
          {
            iFoundObjChild = i;
            break;
          }
        }
      }
    }

    // if exposed object not found, ignore parameter
    if (iFoundObjRoot < 0 && iFoundObjChild < 0)
      continue;

    // store the exposed parameter information
    xiiExposedPrefabParameterDesc& paramdesc = exposedParams.ExpandAndGetRef();
    paramdesc.m_sExposeName.Assign(esp.m_sName.GetData());
    paramdesc.m_uiWorldReaderChildObject = (iFoundObjChild >= 0) ? 1 : 0;
    paramdesc.m_uiWorldReaderObjectIndex = (iFoundObjChild >= 0) ? iFoundObjChild : iFoundObjRoot;
    paramdesc.m_sComponentType.Clear();

    if (pComponenType)
    {
      paramdesc.m_sComponentType.Assign(pComponenType->GetTypeName());
    }

    paramdesc.m_sProperty.Assign(esp.m_sPropertyPath.GetData());
  }

  exposedParams.Sort([](const xiiExposedPrefabParameterDesc& lhs, const xiiExposedPrefabParameterDesc& rhs) -> bool { return lhs.m_sExposeName.GetHash() < rhs.m_sExposeName.GetHash(); });

  file << exposedParams.GetCount();

  for (const auto& ep : exposedParams)
  {
    ep.Save(file);
  }
}

void xiiSceneContext::OnThumbnailViewContextCreated()
{
  // make sure there is ambient light in the thumbnails
  // TODO: should check whether this is a prefab (info currently not available in xiiSceneContext)
  RemoveAmbientLight();
  AddAmbientLight(false, true);
}

void xiiSceneContext::OnDestroyThumbnailViewContext()
{
  RemoveAmbientLight();
}

void xiiSceneContext::UpdateDocumentContext()
{
  SUPER::UpdateDocumentContext();
  xiiGameStateBase* pState = GetGameState();
  if (pState && pState->WasQuitRequested())
  {
    xiiGameApplicationBase::GetGameApplicationBaseInstance()->DeactivateGameState();

    xiiGameModeMsgToEditor msgToEd;
    msgToEd.m_DocumentGuid = GetDocumentGuid();
    msgToEd.m_bRunningPTG  = false;

    SendProcessMessage(&msgToEd);
  }
}

xiiGameObjectHandle xiiSceneContext::ResolveStringToGameObjectHandle(const void* pString, xiiComponentHandle hThis, const char* szProperty) const
{
  // Test if the component is a direct part of this scene or one of its layers.
  if (m_Context.m_ComponentMap.GetGuid(hThis).IsValid())
  {
    return SUPER::ResolveStringToGameObjectHandle(pString, hThis, szProperty);
  }
  for (const xiiLayerContext* pLayer : m_Layers)
  {
    if (pLayer)
    {
      if (pLayer->m_Context.m_ComponentMap.GetGuid(hThis).IsValid())
      {
        return pLayer->ResolveStringToGameObjectHandle(pString, hThis, szProperty);
      }
    }
  }

  // Component not found - it is probably an engine prefab instance part.
  // Walk up the hierarchy and find a game object that belongs to the scene or layer.
  xiiComponent* pComponent = nullptr;
  if (!GetWorld()->TryGetComponent<xiiComponent>(hThis, pComponent))
    return {};

  const xiiGameObject* pParent = pComponent->GetOwner();
  while (pParent)
  {
    if (m_Context.m_GameObjectMap.GetGuid(pParent->GetHandle()).IsValid())
    {
      return SUPER::ResolveStringToGameObjectHandle(pString, hThis, szProperty);
    }
    for (const xiiLayerContext* pLayer : m_Layers)
    {
      if (pLayer)
      {
        if (pLayer->m_Context.m_GameObjectMap.GetGuid(pParent->GetHandle()).IsValid())
        {
          return pLayer->ResolveStringToGameObjectHandle(pString, hThis, szProperty);
        }
      }
    }
    pParent = pParent->GetParent();
  }

  xiiLog::Error("Game object reference could not be resolved. Component source was not found.");
  return xiiGameObjectHandle();
}

bool xiiSceneContext::UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext)
{
  const xiiBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  xiiSceneViewContext* pMaterialViewContext = static_cast<xiiSceneViewContext*>(pThumbnailViewContext);
  const bool           result               = pMaterialViewContext->UpdateThumbnailCamera(bounds);

  return result;
}

void xiiSceneContext::AddAmbientLight(bool bSetEditorTag, bool bForce)
{
  if (!m_hSkyLight.IsInvalidated() || !m_hDirectionalLight.IsInvalidated())
    return;

  XII_LOCK(GetWorld()->GetWriteMarker());

  // Delay adding ambient light until the scene isn't empty, to prevent adding two skylights
  if (!bForce && GetWorld()->GetObjectCount() == 0)
    return;

  xiiSkyLightComponentManager* pSkyMan = GetWorld()->GetComponentManager<xiiSkyLightComponentManager>();
  if (pSkyMan == nullptr || pSkyMan->GetSingletonComponent() == nullptr)
  {
    // only create a skylight, if there is none yet

    xiiGameObjectDesc obj;
    obj.m_sName.Assign("Sky Light");

    if (bSetEditorTag)
    {
      const xiiTag& tagEditor = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
      obj.m_Tags.Set(tagEditor); // to prevent it from being exported
    }

    xiiGameObject* pObj;
    m_hSkyLight = GetWorld()->CreateObject(obj, pObj);


    xiiSkyLightComponent* pSkyLight = nullptr;
    xiiSkyLightComponent::CreateComponent(pObj, pSkyLight);
    pSkyLight->SetCubeMapFile("{ 0b202e08-a64f-465d-b38e-15b81d161822 }");
    pSkyLight->SetReflectionProbeMode(xiiReflectionProbeMode::Static);
  }

  {
    xiiGameObjectDesc obj;
    obj.m_sName.Assign("Ambient Light");

    obj.m_LocalRotation.SetFromEulerAngles(xiiAngle::Degree(-14.510815f), xiiAngle::Degree(43.07951f), xiiAngle::Degree(93.223808f));

    if (bSetEditorTag)
    {
      const xiiTag& tagEditor = xiiTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
      obj.m_Tags.Set(tagEditor); // to prevent it from being exported
    }

    xiiGameObject* pLight;
    m_hDirectionalLight = GetWorld()->CreateObject(obj, pLight);

    xiiDirectionalLightComponent* pDirLight = nullptr;
    xiiDirectionalLightComponent::CreateComponent(pLight, pDirLight);
    pDirLight->SetIntensity(10.0f);
  }
}

void xiiSceneContext::RemoveAmbientLight()
{
  XII_LOCK(GetWorld()->GetWriteMarker());

  if (!m_hSkyLight.IsInvalidated())
  {
    // make sure to remove the object RIGHT NOW, otherwise it may still exist during scene export (without the "Editor" tag)
    GetWorld()->DeleteObjectNow(m_hSkyLight);
    m_hSkyLight.Invalidate();
  }

  if (!m_hDirectionalLight.IsInvalidated())
  {
    // make sure to remove the object RIGHT NOW, otherwise it may still exist during scene export (without the "Editor" tag)
    GetWorld()->DeleteObjectNow(m_hDirectionalLight);
    m_hDirectionalLight.Invalidate();
  }
}

const xiiEngineProcessDocumentContext* xiiSceneContext::GetActiveDocumentContext() const
{
  if (m_ActiveLayer == GetDocumentGuid())
  {
    return this;
  }

  for (const xiiLayerContext* pLayer : m_Layers)
  {
    if (pLayer && m_ActiveLayer == pLayer->GetDocumentGuid())
    {
      return pLayer;
    }
  }

  XII_REPORT_FAILURE("Active layer does not exist.");
  return this;
}

xiiEngineProcessDocumentContext* xiiSceneContext::GetActiveDocumentContext()
{
  return const_cast<xiiEngineProcessDocumentContext*>(const_cast<const xiiSceneContext*>(this)->GetActiveDocumentContext());
}

const xiiWorldRttiConverterContext& xiiSceneContext::GetActiveContext() const
{
  return GetActiveDocumentContext()->m_Context;
}

xiiWorldRttiConverterContext& xiiSceneContext::GetActiveContext()
{
  return const_cast<xiiWorldRttiConverterContext&>(const_cast<const xiiSceneContext*>(this)->GetActiveContext());
}

xiiWorldRttiConverterContext* xiiSceneContext::GetContextForLayer(const xiiUuid& layerGuid)
{
  if (layerGuid == GetDocumentGuid())
    return &m_Context;

  for (xiiLayerContext* pLayer : m_Layers)
  {
    if (pLayer && layerGuid == pLayer->GetDocumentGuid())
    {
      return &pLayer->m_Context;
    }
  }
  return nullptr;
}

xiiArrayPtr<xiiWorldRttiConverterContext*> xiiSceneContext::GetAllContexts()
{
  return m_Contexts;
}

void xiiSceneContext::HandleExposedPropertiesMsg(const xiiExposedDocumentObjectPropertiesMsgToEngine* pMsg)
{
  m_ExposedSceneProperties = pMsg->m_Properties;
}

void xiiSceneContext::HandleSceneGeometryMsg(const xiiExportSceneGeometryMsgToEngine* pMsg)
{
  xiiWorldGeoExtractionUtil::MeshObjectList objects;

  xiiTagSet excludeTags;
  excludeTags.SetByName("Editor");

  if (pMsg->m_bSelectionOnly)
    xiiWorldGeoExtractionUtil::ExtractWorldGeometry(objects, *m_pWorld, static_cast<xiiWorldGeoExtractionUtil::ExtractionMode>(pMsg->m_iExtractionMode), m_SelectionWithChildren);
  else
    xiiWorldGeoExtractionUtil::ExtractWorldGeometry(objects, *m_pWorld, static_cast<xiiWorldGeoExtractionUtil::ExtractionMode>(pMsg->m_iExtractionMode), &excludeTags);

  xiiWorldGeoExtractionUtil::WriteWorldGeometryToOBJ(pMsg->m_sOutputFile, objects, pMsg->m_Transform);
}

void xiiSceneContext::HandlePullObjectStateMsg(const xiiPullObjectStateMsgToEngine* pMsg)
{
  if (!m_pWorld->GetWorldSimulationEnabled())
    return;

  const xiiWorld* pWorld = GetWorld();
  XII_LOCK(pWorld->GetReadMarker());

  const auto& objectMapper = GetActiveContext().m_GameObjectMap;

  m_PushObjectStateMsg.m_ObjectStates.Reserve(m_PushObjectStateMsg.m_ObjectStates.GetCount() + m_SelectionWithChildren.GetCount());

  for (xiiGameObjectHandle hObject : m_SelectionWithChildren)
  {
    const xiiGameObject* pObject = nullptr;
    if (!pWorld->TryGetObject(hObject, pObject))
      continue;

    xiiUuid objectGuid = objectMapper.GetGuid(hObject);
    bool    bAdjust    = false;

    if (!objectGuid.IsValid())
    {
      // this must be an object created on the runtime side, try to match it to some editor object
      // we only try the direct parent, more steps than that are not allowed

      const xiiGameObject* pParentObject = pObject->GetParent();
      if (pParentObject == nullptr)
        continue;

      // if the parent has more than one child, remapping the position from the child to the parent is not possible, so skip those
      if (pParentObject->GetChildCount() > 1)
        continue;

      auto parentGuid = objectMapper.GetGuid(pParentObject->GetHandle());

      if (!parentGuid.IsValid())
        continue;

      objectGuid = parentGuid;
      bAdjust    = true;

      for (xiiUInt32 i = 0; i < m_PushObjectStateMsg.m_ObjectStates.GetCount(); ++i)
      {
        if (m_PushObjectStateMsg.m_ObjectStates[i].m_ObjectGuid == objectGuid)
        {
          m_PushObjectStateMsg.m_ObjectStates.RemoveAtAndCopy(i);
          break;
        }
      }
    }

    {
      auto& state = m_PushObjectStateMsg.m_ObjectStates.ExpandAndGetRef();

      state.m_LayerGuid                  = m_ActiveLayer;
      state.m_ObjectGuid                 = objectGuid;
      state.m_bAdjustFromPrefabRootChild = bAdjust;
      state.m_vPosition                  = pObject->GetGlobalPosition();
      state.m_qRotation                  = pObject->GetGlobalRotation();

      xiiMsgRetrieveBoneState msg;
      pObject->SendMessage(msg);

      state.m_BoneTransforms = msg.m_BoneTransforms;
    }
  }

  // the return message is sent after the simulation has stopped
}
