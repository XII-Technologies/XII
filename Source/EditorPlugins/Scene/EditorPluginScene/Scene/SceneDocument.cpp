#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorFramework/Preferences/QuadViewPreferences.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <EditorPluginScene/Commands/SceneCommands.h>
#include <EditorPluginScene/Dialogs/DeltaTransformDlg.moc.h>
#include <EditorPluginScene/Dialogs/DuplicateDlg.moc.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <QClipboard>
#include <RendererCore/Components/CameraComponent.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectDirectAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSceneDocument, 7, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiSceneDocument_PropertyMetaStateEventHandler(xiiPropertyMetaStateEvent& e)
{
  static const xiiRTTI* pRtti     = xiiRTTI::FindTypeByName("xiiGameObject");
  const char*           szDocType = e.m_pObject->GetDocumentObjectManager()->GetDocument()->GetDocumentTypeName();

  if (!xiiStringUtils::IsEqual(szDocType, "Prefab"))
    return;

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  auto pParent = e.m_pObject->GetParent();
  if (pParent != nullptr)
  {
    if (pParent->GetTypeAccessor().GetType() == pRtti)
      return;
  }

  const xiiString name = e.m_pObject->GetTypeAccessor().GetValue("Name").ConvertTo<xiiString>();
  if (name != "<Prefab-Root>")
    return;

  auto& props                               = *e.m_pPropertyStates;
  props["Name"].m_sNewLabelText             = "Prefab.NameLabel";
  props["Active"].m_Visibility              = xiiPropertyUiState::Invisible;
  props["LocalPosition"].m_Visibility       = xiiPropertyUiState::Invisible;
  props["LocalRotation"].m_Visibility       = xiiPropertyUiState::Invisible;
  props["LocalScaling"].m_Visibility        = xiiPropertyUiState::Invisible;
  props["LocalUniformScaling"].m_Visibility = xiiPropertyUiState::Invisible;
  props["GlobalKey"].m_Visibility           = xiiPropertyUiState::Invisible;
  props["Tags"].m_Visibility                = xiiPropertyUiState::Invisible;
}

xiiSceneDocument::xiiSceneDocument(const char* szDocumentPath, DocumentType DocumentType) :
  xiiGameObjectDocument(szDocumentPath, XII_DEFAULT_NEW(xiiSceneObjectManager))
{
  m_DocumentType = DocumentType;
  m_GameMode     = GameMode::Off;
  SetAddAmbientLight(IsPrefab());

  m_GameModeData[GameMode::Off].m_bRenderSelectionOverlay = true;
  m_GameModeData[GameMode::Off].m_bRenderShapeIcons       = true;
  m_GameModeData[GameMode::Off].m_bRenderVisualizers      = true;

  m_GameModeData[GameMode::Simulate].m_bRenderSelectionOverlay = false;
  m_GameModeData[GameMode::Simulate].m_bRenderShapeIcons       = false;
  m_GameModeData[GameMode::Simulate].m_bRenderVisualizers      = false;

  m_GameModeData[GameMode::Play].m_bRenderSelectionOverlay = false;
  m_GameModeData[GameMode::Play].m_bRenderShapeIcons       = false;
  m_GameModeData[GameMode::Play].m_bRenderVisualizers      = false;
}


void xiiSceneDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  // (Local mirror only mirrors settings)
  m_ObjectMirror.SetFilterFunction([pManager = GetObjectManager()](const xiiDocumentObject* pObject, const char* szProperty) -> bool { return pManager->IsUnderRootProperty("Settings", pObject, szProperty); });
  // (Remote IPC mirror only sends scene)
  m_Mirror.SetFilterFunction([pManager = GetObjectManager()](const xiiDocumentObject* pObject, const char* szProperty) -> bool { return pManager->IsUnderRootProperty("Children", pObject, szProperty); });

  SUPER::InitializeAfterLoading(bFirstTimeCreation);
  EnsureSettingsObjectExist();

  m_DocumentObjectMetaData->m_DataModifiedEvent.AddEventHandler(xiiMakeDelegate(&xiiSceneDocument::DocumentObjectMetaDataEventHandler, this));
  xiiToolsProject::s_Events.AddEventHandler(xiiMakeDelegate(&xiiSceneDocument::ToolsProjectEventHandler, this));
  xiiEditorEngineProcessConnection::GetSingleton()->s_Events.AddEventHandler(xiiMakeDelegate(&xiiSceneDocument::EngineConnectionEventHandler, this));

  m_ObjectMirror.InitSender(GetObjectManager());
  m_ObjectMirror.InitReceiver(&m_Context);
  m_ObjectMirror.SendDocument();
}

xiiSceneDocument::~xiiSceneDocument()
{
  m_DocumentObjectMetaData->m_DataModifiedEvent.RemoveEventHandler(xiiMakeDelegate(&xiiSceneDocument::DocumentObjectMetaDataEventHandler, this));

  xiiToolsProject::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiSceneDocument::ToolsProjectEventHandler, this));

  xiiEditorEngineProcessConnection::GetSingleton()->s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiSceneDocument::EngineConnectionEventHandler, this));

  m_ObjectMirror.Clear();
  m_ObjectMirror.DeInit();
}

void xiiSceneDocument::GroupSelection()
{
  const auto&     sel    = GetSelectionManager()->GetSelection();
  const xiiUInt32 numSel = sel.GetCount();
  if (numSel <= 1)
    return;

  xiiVec3                  vCenter(0.0f);
  const xiiDocumentObject* pCommonParent = sel[0]->GetParent();

  for (const auto& item : sel)
  {
    vCenter += GetGlobalTransform(item).m_vPosition;

    if (pCommonParent != item->GetParent())
    {
      pCommonParent = nullptr;
    }
  }

  vCenter /= numSel;

  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Group Selection");

  xiiUuid groupObj;
  groupObj.CreateNewUuid();

  xiiAddObjectCommand cmdAdd;
  cmdAdd.m_NewObjectGuid   = groupObj;
  cmdAdd.m_pType           = xiiGetStaticRTTI<xiiGameObject>();
  cmdAdd.m_Index           = -1;
  cmdAdd.m_sParentProperty = "Children";

  pHistory->AddCommand(cmdAdd);

  // put the new group object under the shared parent
  if (pCommonParent != nullptr)
  {
    xiiMoveObjectCommand cmdMove;
    cmdMove.m_NewParent       = pCommonParent->GetGuid();
    cmdMove.m_Index           = -1;
    cmdMove.m_sParentProperty = "Children";

    cmdMove.m_Object = cmdAdd.m_NewObjectGuid;
    pHistory->AddCommand(cmdMove);
  }

  auto pGroupObject = GetObjectManager()->GetObject(cmdAdd.m_NewObjectGuid);
  SetGlobalTransform(pGroupObject, xiiTransform(vCenter), TransformationChanges::Translation);

  xiiMoveObjectCommand cmdMove;
  cmdMove.m_NewParent       = cmdAdd.m_NewObjectGuid;
  cmdMove.m_Index           = -1;
  cmdMove.m_sParentProperty = "Children";

  for (const auto& item : sel)
  {
    cmdMove.m_Object = item->GetGuid();
    pHistory->AddCommand(cmdMove);
  }

  pHistory->FinishTransaction();

  const xiiDocumentObject* pGroupObj = GetObjectManager()->GetObject(groupObj);

  GetSelectionManager()->SetSelection(pGroupObj);

  ShowDocumentStatus(xiiFmt("Grouped {} objects", numSel));
}


void xiiSceneDocument::DuplicateSpecial()
{
  if (GetSelectionManager()->IsSelectionEmpty())
    return;

  xiiQtDuplicateDlg dlg(nullptr);
  if (dlg.exec() == QDialog::Rejected)
    return;

  xiiMap<xiiUuid, xiiUuid> parents;

  xiiAbstractObjectGraph graph;
  CopySelectedObjects(graph, &parents);

  xiiStringBuilder temp, tmp1, tmp2;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("{0}={1};", xiiConversionUtils::ToString(it.Key(), tmp1), xiiConversionUtils::ToString(it.Value(), tmp2));
  }

  // Serialize to string
  xiiContiguousMemoryStreamStorage streamStorage;
  xiiMemoryStreamWriter            memoryWriter(&streamStorage);

  xiiAbstractGraphDdlSerializer::Write(memoryWriter, &graph);
  memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

  xiiDuplicateObjectsCommand cmd;
  cmd.m_sGraphTextFormat         = (const char*)streamStorage.GetData();
  cmd.m_sParentNodes             = temp;
  cmd.m_uiNumberOfCopies         = dlg.s_uiNumberOfCopies;
  cmd.m_vAccumulativeTranslation = dlg.s_vTranslationStep;
  cmd.m_vAccumulativeRotation    = dlg.s_vRotationStep;
  cmd.m_vRandomRotation          = dlg.s_vRandomRotation;
  cmd.m_vRandomTranslation       = dlg.s_vRandomTranslation;
  cmd.m_bGroupDuplicates         = dlg.s_bGroupCopies;
  cmd.m_iRevolveAxis             = dlg.s_iRevolveAxis;
  cmd.m_fRevolveRadius           = dlg.s_fRevolveRadius;
  cmd.m_RevolveStartAngle        = xiiAngle::Degree(dlg.s_iRevolveStartAngle);
  cmd.m_RevolveAngleStep         = xiiAngle::Degree(dlg.s_iRevolveAngleStep);

  auto history = GetCommandHistory();

  history->StartTransaction("Duplicate Special");

  if (history->AddCommand(cmd).m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}


void xiiSceneDocument::DeltaTransform()
{
  if (GetSelectionManager()->IsSelectionEmpty())
    return;

  xiiQtDeltaTransformDlg dlg(nullptr, this);
  dlg.exec();
}

void xiiSceneDocument::SnapObjectToCamera()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  const auto& ctxt = xiiQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr)
    return;

  if (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective != xiiSceneViewPerspective::Perspective)
  {
    ShowDocumentStatus("Note: This operation can only be performed in perspective views.");
    return;
  }

  const auto& camera = ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  xiiMat3 mRot;

  xiiTransform transform;
  transform.m_vScale.Set(1.0f);
  transform.m_vPosition = camera.GetCenterPosition();
  mRot.SetColumn(0, camera.GetCenterDirForwards());
  mRot.SetColumn(1, camera.GetCenterDirRight());
  mRot.SetColumn(2, camera.GetCenterDirUp());
  transform.m_qRotation.SetFromMat3(mRot);

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Snap Object to Camera");
  {
    for (const xiiDocumentObject* pObject : selection)
    {
      SetGlobalTransform(pObject, transform, TransformationChanges::Translation | TransformationChanges::Rotation);
    }
  }
  pHistory->FinishTransaction();
}


void xiiSceneDocument::AttachToObject()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  const auto& ctxt = xiiQtEngineViewWidget::GetInteractionContext();
  if (ctxt.m_pLastHoveredViewWidget == nullptr || ctxt.m_pLastPickingResult == nullptr || !ctxt.m_pLastPickingResult->m_PickedObject.IsValid())
    return;

  if (GetObjectManager()->GetObject(ctxt.m_pLastPickingResult->m_PickedObject) == nullptr)
  {
    xiiQtUiServices::GetSingleton()->MessageBoxStatus(xiiStatus(XII_FAILURE), "Target object belongs to a different document.");
    return;
  }

  xiiMoveObjectCommand cmd;
  cmd.m_sParentProperty = "Children";
  cmd.m_NewParent       = ctxt.m_pLastPickingResult->m_PickedObject;
  cmd.m_Index           = -1;

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Attach to Object");
  {
    for (const xiiDocumentObject* pObject : selection)
    {
      cmd.m_Object = pObject->GetGuid();

      auto res = pHistory->AddCommand(cmd);
      if (res.Failed())
      {
        xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Attach to object failed");
        pHistory->CancelTransaction();
        return;
      }
    }
  }
  pHistory->FinishTransaction();
}

void xiiSceneDocument::DetachFromParent()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return;

  xiiMoveObjectCommand cmd;
  cmd.m_sParentProperty = "Children";

  auto* pHistory = GetCommandHistory();

  pHistory->StartTransaction("Detach from Parent");
  {
    for (const xiiDocumentObject* pObject : selection)
    {
      cmd.m_Object = pObject->GetGuid();

      auto res = pHistory->AddCommand(cmd);
      if (res.Failed())
      {
        xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Detach from parent failed");
        pHistory->CancelTransaction();
        return;
      }
    }
  }
  pHistory->FinishTransaction();

  ShowDocumentStatus(xiiFmt("Detached {} objects", selection.GetCount()));

  // reapply the selection to fix tree views etc. after the re-parenting
  xiiDeque<const xiiDocumentObject*> prevSelection = selection;
  GetSelectionManager()->Clear();
  GetSelectionManager()->SetSelection(prevSelection);
}

void xiiSceneDocument::CopyReference()
{
  if (GetSelectionManager()->GetSelection().GetCount() != 1)
    return;

  const xiiUuid guid = GetSelectionManager()->GetSelection()[0]->GetGuid();

  xiiStringBuilder sGuid;
  xiiConversionUtils::ToString(guid, sGuid);

  QApplication::clipboard()->setText(sGuid.GetData());

  xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(xiiFmt("Copied Object Reference: {}", sGuid), xiiTime::Seconds(5));
}

xiiStatus xiiSceneDocument::CreateEmptyObject(bool bAttachToParent, bool bAtPickedPosition)
{
  auto history = GetCommandHistory();

  history->StartTransaction("Create Node");

  xiiAddObjectCommand cmdAdd;
  cmdAdd.m_pType           = xiiGetStaticRTTI<xiiGameObject>();
  cmdAdd.m_sParentProperty = "Children";
  cmdAdd.m_Index           = -1;

  xiiUuid NewNode;

  const auto& Sel = GetSelectionManager()->GetSelection();

  if (Sel.IsEmpty() || !bAttachToParent)
  {
    cmdAdd.m_NewObjectGuid.CreateNewUuid();
    NewNode = cmdAdd.m_NewObjectGuid;

    auto res = history->AddCommand(cmdAdd);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }
  else
  {
    cmdAdd.m_NewObjectGuid.CreateNewUuid();
    NewNode = cmdAdd.m_NewObjectGuid;

    cmdAdd.m_Parent = Sel[0]->GetGuid();
    auto res        = history->AddCommand(cmdAdd);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }

  const auto& ctxt = xiiQtEngineViewWidget::GetInteractionContext();

  if (!bAttachToParent && bAtPickedPosition && ctxt.m_pLastPickingResult && !ctxt.m_pLastPickingResult->m_vPickedPosition.IsNaN())
  {
    xiiVec3 position = ctxt.m_pLastPickingResult->m_vPickedPosition;

    xiiSnapProvider::SnapTranslation(position);

    xiiSetObjectPropertyCommand cmdSet;
    cmdSet.m_NewValue  = position;
    cmdSet.m_Object    = NewNode;
    cmdSet.m_sProperty = "LocalPosition";

    auto res = history->AddCommand(cmdSet);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }

  // Add a dummy shape icon component, which enables picking
  {
    xiiAddObjectCommand cmdAdd;
    cmdAdd.m_pType           = xiiRTTI::FindTypeByName("xiiShapeIconComponent");
    cmdAdd.m_sParentProperty = "Components";
    cmdAdd.m_Index           = -1;
    cmdAdd.m_Parent          = NewNode;

    auto res = history->AddCommand(cmdAdd);
  }

  history->FinishTransaction();

  GetSelectionManager()->SetSelection(GetObjectManager()->GetObject(NewNode));
  return xiiStatus(XII_SUCCESS);
}

void xiiSceneDocument::DuplicateSelection()
{
  xiiMap<xiiUuid, xiiUuid> parents;

  xiiAbstractObjectGraph graph;
  CopySelectedObjects(graph, &parents);

  xiiStringBuilder temp, tmp1, tmp2;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("{0}={1};", xiiConversionUtils::ToString(it.Key(), tmp1), xiiConversionUtils::ToString(it.Value(), tmp2));
  }

  // Serialize to string
  xiiContiguousMemoryStreamStorage streamStorage;
  xiiMemoryStreamWriter            memoryWriter(&streamStorage);

  xiiAbstractGraphDdlSerializer::Write(memoryWriter, &graph);
  memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

  xiiDuplicateObjectsCommand cmd;
  cmd.m_sGraphTextFormat = (const char*)streamStorage.GetData();
  cmd.m_sParentNodes     = temp;

  auto history = GetCommandHistory();

  history->StartTransaction("Duplicate Selection");

  if (history->AddCommand(cmd).m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}

void xiiSceneDocument::ShowOrHideSelectedObjects(ShowOrHide action)
{
  const bool bHide = action == ShowOrHide::Hide;

  auto sel = GetSelectionManager()->GetSelection();

  for (auto pItem : sel)
  {
    if (!pItem->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
      continue;

    ApplyRecursive(pItem, [this, bHide](const xiiDocumentObject* pObj) {
      // if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
      // return;

      auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObj->GetGuid());
      if (pMeta->m_bHidden != bHide)
      {
        pMeta->m_bHidden = bHide;
        m_DocumentObjectMetaData->EndModifyMetaData(xiiDocumentObjectMetaData::HiddenFlag);
      }
      else
        m_DocumentObjectMetaData->EndModifyMetaData(0); });
  }
}

void xiiSceneDocument::HideUnselectedObjects()
{
  ShowOrHideAllObjects(ShowOrHide::Hide);

  ShowOrHideSelectedObjects(ShowOrHide::Show);
}

void xiiSceneDocument::SetGameMode(GameMode::Enum mode)
{
  if (m_GameMode == mode)
    return;

  // store settings of recently active mode
  m_GameModeData[m_GameMode] = m_CurrentMode;

  m_GameMode = mode;

  switch (m_GameMode)
  {
    case GameMode::Off:
      ShowDocumentStatus("Game Mode: Off");
      break;
    case GameMode::Simulate:
      ShowDocumentStatus("Game Mode: Simulate");
      break;
    case GameMode::Play:
      ShowDocumentStatus("Game Mode: Play");
      break;
  }

  SetRenderSelectionOverlay(m_GameModeData[m_GameMode].m_bRenderSelectionOverlay);
  SetRenderShapeIcons(m_GameModeData[m_GameMode].m_bRenderShapeIcons);
  SetRenderVisualizers(m_GameModeData[m_GameMode].m_bRenderVisualizers);

  if (m_GameMode == GameMode::Off)
  {
    // reset the game world
    SendGameWorldToEngine();
  }

  xiiGameObjectEvent e;
  e.m_Type = xiiGameObjectEvent::Type::GameModeChanged;
  m_GameObjectEvents.Broadcast(e);

  ScheduleSendObjectSelection();
}

xiiStatus xiiSceneDocument::CreatePrefabDocumentFromSelection(const char* szFile, const xiiRTTI* pRootType, xiiDelegate<void(xiiAbstractObjectNode*)> adjustGraphNodeCB /* = {} */, xiiDelegate<void(xiiDocumentObject*)> adjustNewNodesCB /* = {} */, xiiDelegate<void(xiiAbstractObjectGraph& graph, xiiDynamicArray<xiiAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB /* = {} */)
{
  XII_ASSERT_DEV(!adjustGraphNodeCB.IsValid(), "Not allowed");
  XII_ASSERT_DEV(!adjustNewNodesCB.IsValid(), "Not allowed");
  XII_ASSERT_DEV(!finalizeGraphCB.IsValid(), "Not allowed");

  auto Selection = GetSelectionManager()->GetTopLevelSelection(pRootType);

  if (Selection.IsEmpty())
    return xiiStatus("To create a prefab, the selection must not be empty");

  const xiiTransform tReference = QueryLocalTransform(Selection.PeekBack());

  xiiVariantArray varChildren;

  auto centerNodes = [tReference, &varChildren](xiiAbstractObjectNode* pGraphNode) {
    if (auto pPosition = pGraphNode->FindProperty("LocalPosition"))
    {
      xiiVec3 pos = pPosition->m_Value.ConvertTo<xiiVec3>();
      pos -= tReference.m_vPosition;

      pGraphNode->ChangeProperty("LocalPosition", pos);
    }

    if (auto pRotation = pGraphNode->FindProperty("LocalRotation"))
    {
      xiiQuat rot = pRotation->m_Value.ConvertTo<xiiQuat>();
      rot         = -tReference.m_qRotation * rot;

      pGraphNode->ChangeProperty("LocalRotation", rot);
    }

    varChildren.PushBack(pGraphNode->GetGuid());
  };

  auto adjustResult = [tReference, this](xiiDocumentObject* pObject) {
    const xiiTransform tOld = QueryLocalTransform(pObject);

    xiiSetObjectPropertyCommand cmd;
    cmd.m_Object = pObject->GetGuid();

    cmd.m_sProperty = "LocalPosition";
    cmd.m_NewValue  = tOld.m_vPosition + tReference.m_vPosition;
    GetCommandHistory()->AddCommand(cmd);

    cmd.m_sProperty = "LocalRotation";
    cmd.m_NewValue  = tReference.m_qRotation * tOld.m_qRotation;
    GetCommandHistory()->AddCommand(cmd);
  };

  auto finalizeGraph = [this, &varChildren](xiiAbstractObjectGraph& graph, xiiDynamicArray<xiiAbstractObjectNode*>& graphRootNodes) {
    if (graphRootNodes.GetCount() == 1)
    {
      graphRootNodes[0]->ChangeProperty("Name", "<Prefab-Root>");
    }
    else
    {
      const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiGameObject>();

      xiiAbstractObjectNode* pRoot = graph.AddNode(xiiUuid::CreateUuid(), pRtti->GetTypeName(), pRtti->GetTypeVersion());
      pRoot->AddProperty("Name", "<Prefab-Root>");
      pRoot->AddProperty("Children", varChildren);

      graphRootNodes.Clear();
      graphRootNodes.PushBack(pRoot);
    }
  };

  return SUPER::CreatePrefabDocumentFromSelection(szFile, pRootType, centerNodes, adjustResult, finalizeGraph);
}

bool xiiSceneDocument::CanEngineProcessBeRestarted() const
{
  return m_GameMode == GameMode::Off;
}

void xiiSceneDocument::StartSimulateWorld()
{
  if (m_GameMode != GameMode::Off)
  {
    StopGameMode();
    return;
  }

  {
    xiiGameObjectDocumentEvent e;
    e.m_Type      = xiiGameObjectDocumentEvent::Type::GameMode_StartingSimulate;
    e.m_pDocument = this;
    s_GameObjectDocumentEvents.Broadcast(e);
  }

  SetGameMode(GameMode::Simulate);
}


void xiiSceneDocument::TriggerGameModePlay(bool bUsePickedPositionAsStart)
{
  if (m_GameMode != GameMode::Off)
  {
    StopGameMode();
    return;
  }

  {
    xiiGameObjectDocumentEvent e;
    e.m_Type      = xiiGameObjectDocumentEvent::Type::GameMode_StartingPlay;
    e.m_pDocument = this;
    s_GameObjectDocumentEvents.Broadcast(e);
  }

  UpdateObjectDebugTargets();

  // attempt to start PTG
  // do not change state here
  {
    xiiGameModeMsgToEngine msg;
    msg.m_bEnablePTG        = true;
    msg.m_bUseStartPosition = false;

    if (bUsePickedPositionAsStart)
    {
      const auto& ctxt = xiiQtEngineViewWidget::GetInteractionContext();

      if (ctxt.m_pLastHoveredViewWidget != nullptr && ctxt.m_pLastHoveredViewWidget->GetDocumentWindow()->GetDocument() == this)
      {
        msg.m_bUseStartPosition = true;
        msg.m_vStartPosition    = ctxt.m_pLastPickingResult->m_vPickedPosition;

        xiiVec3 vPickDir = ctxt.m_pLastPickingResult->m_vPickedPosition - ctxt.m_pLastPickingResult->m_vPickingRayStart;
        vPickDir.z       = 0;
        vPickDir.NormalizeIfNotZero(xiiVec3(1, 0, 0)).IgnoreResult();

        msg.m_vStartDirection = vPickDir;
      }
    }

    GetEditorEngineConnection()->SendMessage(&msg);
  }
}


bool xiiSceneDocument::StopGameMode()
{
  if (m_GameMode == GameMode::Off)
    return false;

  if (m_GameMode == GameMode::Simulate)
  {
    // we can set that state immediately
    SetGameMode(GameMode::Off);
  }

  if (m_GameMode == GameMode::Play)
  {
    // attempt to stop PTG
    // do not change any state, that will be done by the response msg
    {
      xiiGameModeMsgToEngine msg;
      msg.m_bEnablePTG = false;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
  }

  {
    xiiGameObjectDocumentEvent e;
    e.m_Type      = xiiGameObjectDocumentEvent::Type::GameMode_Stopped;
    e.m_pDocument = this;
    s_GameObjectDocumentEvents.Broadcast(e);
  }

  return true;
}

void xiiSceneDocument::ShowOrHideAllObjects(ShowOrHide action)
{
  const bool bHide = action == ShowOrHide::Hide;

  ApplyRecursive(GetObjectManager()->GetRootObject(), [this, bHide](const xiiDocumentObject* pObj) {
    // if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
    // return;

    xiiUInt32 uiFlags = 0;

    auto pMeta = m_DocumentObjectMetaData->BeginModifyMetaData(pObj->GetGuid());

    if (pMeta->m_bHidden != bHide)
    {
      pMeta->m_bHidden = bHide;
      uiFlags = xiiDocumentObjectMetaData::HiddenFlag;
    }

    m_DocumentObjectMetaData->EndModifyMetaData(uiFlags); });
}
void xiiSceneDocument::GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/xiiEditor.xiiAbstractGraph");
}

bool xiiSceneDocument::CopySelectedObjects(xiiAbstractObjectGraph& graph, xiiStringBuilder& out_MimeType) const
{
  out_MimeType = "application/xiiEditor.xiiAbstractGraph";
  return CopySelectedObjects(graph, nullptr);
}

bool xiiSceneDocument::CopySelectedObjects(xiiAbstractObjectGraph& graph, xiiMap<xiiUuid, xiiUuid>* out_pParents) const
{
  if (GetSelectionManager()->GetSelection().GetCount() == 0)
    return false;

  // Serialize selection to graph
  auto Selection = GetSelectionManager()->GetTopLevelSelection();

  xiiDocumentObjectConverterWriter writer(&graph, GetObjectManager());

  // TODO: objects are required to be named root but this is not enforced or obvious by the interface.
  for (xiiUInt32 i = 0; i < Selection.GetCount(); i++)
  {
    auto                   item  = Selection[i];
    xiiAbstractObjectNode* pNode = writer.AddObjectToGraph(item, "root");
    pNode->AddProperty("__GlobalTransform", GetGlobalTransform(item));
    pNode->AddProperty("__Order", i);
  }

  if (out_pParents != nullptr)
  {
    out_pParents->Clear();

    for (auto item : Selection)
    {
      (*out_pParents)[item->GetGuid()] = item->GetParent()->GetGuid();
    }
  }

  AttachMetaDataBeforeSaving(graph);

  return true;
}

bool xiiSceneDocument::PasteAt(const xiiArrayPtr<PasteInfo>& info, const xiiVec3& vPasteAt)
{
  xiiVec3 vAvgPos(0.0f);

  for (const PasteInfo& pi : info)
  {
    if (pi.m_pObject->GetTypeAccessor().GetType() != xiiGetStaticRTTI<xiiGameObject>())
      return false;

    vAvgPos += pi.m_pObject->GetTypeAccessor().GetValue("LocalPosition").Get<xiiVec3>();
  }

  vAvgPos /= info.GetCount();

  for (const PasteInfo& pi : info)
  {
    const xiiVec3 vLocalPos = pi.m_pObject->GetTypeAccessor().GetValue("LocalPosition").Get<xiiVec3>();
    pi.m_pObject->GetTypeAccessor().SetValue("LocalPosition", vLocalPos - vAvgPos + vPasteAt);

    if (pi.m_pParent == nullptr || pi.m_pParent == GetObjectManager()->GetRootObject())
    {
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      GetObjectManager()->AddObject(pi.m_pObject, pi.m_pParent, "Children", pi.m_Index);
    }
  }

  return true;
}

bool xiiSceneDocument::PasteAtOrignalPosition(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph)
{
  for (const PasteInfo& pi : info)
  {
    if (pi.m_pParent == nullptr || pi.m_pParent == GetObjectManager()->GetRootObject())
    {
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      GetObjectManager()->AddObject(pi.m_pObject, pi.m_pParent, "Children", pi.m_Index);
    }
    if (auto* pNode = objectGraph.GetNode(pi.m_pObject->GetGuid()))
    {
      if (auto* pProperty = pNode->FindProperty("__GlobalTransform"))
      {
        if (pProperty->m_Value.IsA<xiiTransform>())
        {
          SetGlobalTransform(pi.m_pObject, pProperty->m_Value.Get<xiiTransform>(), TransformationChanges::All);
        }
      }
    }
  }

  return true;
}

bool xiiSceneDocument::Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  const auto& ctxt = xiiQtEngineViewWidget::GetInteractionContext();

  if (bAllowPickedPosition && ctxt.m_pLastPickingResult && ctxt.m_pLastPickingResult->m_PickedObject.IsValid())
  {
    xiiVec3 pos = ctxt.m_pLastPickingResult->m_vPickedPosition;
    xiiSnapProvider::SnapTranslation(pos);

    if (!PasteAt(info, pos))
      return false;
  }
  else
  {
    if (!PasteAtOrignalPosition(info, objectGraph))
      return false;
  }

  m_DocumentObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);
  m_GameObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);

  // set the pasted objects as the new selection
  {
    auto pSelMan = GetSelectionManager();

    xiiDeque<const xiiDocumentObject*> NewSelection;

    for (const PasteInfo& pi : info)
    {
      NewSelection.PushBack(pi.m_pObject);
    }

    pSelMan->SetSelection(NewSelection);
  }

  return true;
}

bool xiiSceneDocument::DuplicateSelectedObjects(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bSetSelected)
{
  if (!PasteAtOrignalPosition(info, objectGraph))
    return false;

  m_DocumentObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);
  m_GameObjectMetaData->RestoreMetaDataFromAbstractGraph(objectGraph);

  // set the pasted objects as the new selection
  if (bSetSelected)
  {
    auto pSelMan = GetSelectionManager();

    xiiDeque<const xiiDocumentObject*> NewSelection;

    for (const PasteInfo& pi : info)
    {
      NewSelection.PushBack(pi.m_pObject);
    }

    pSelMan->SetSelection(NewSelection);
  }

  return true;
}

void xiiSceneDocument::EnsureSettingsObjectExist()
{
  // Settings object was changed to have a base class and each document type has a different implementation.
  const xiiRTTI* pSettingsType = nullptr;
  switch (m_DocumentType)
  {
    case xiiSceneDocument::DocumentType::Scene:
      pSettingsType = xiiGetStaticRTTI<xiiSceneDocumentSettings>();
      break;
    case xiiSceneDocument::DocumentType::Prefab:
      pSettingsType = xiiGetStaticRTTI<xiiPrefabDocumentSettings>();
      break;
    case xiiSceneDocument::DocumentType::Layer:
      pSettingsType = xiiGetStaticRTTI<xiiLayerDocumentSettings>();
      break;
  }

  auto pRoot = GetObjectManager()->GetRootObject();
  // Use the xiiObjectDirectAccessor instead of calling GetObjectAccessor because we do not want
  // undo ops for this operation.
  xiiObjectDirectAccessor accessor(GetObjectManager());
  xiiVariant              value;
  XII_VERIFY(accessor.xiiObjectAccessorBase::GetValue(pRoot, "Settings", value).Succeeded(), "The scene doc root should have a settings property.");
  xiiUuid id = value.Get<xiiUuid>();
  if (!id.IsValid())
  {
    XII_VERIFY(accessor.xiiObjectAccessorBase::AddObject(pRoot, "Settings", xiiVariant(), pSettingsType, id).Succeeded(), "Adding scene settings object to root failed.");
  }
  else
  {
    xiiDocumentObject* pSettings = GetObjectManager()->GetObject(id);
    XII_VERIFY(pSettings, "Document corrupt, root references a non-existing object");
    if (pSettings->GetType() != pSettingsType)
    {
      accessor.RemoveObject(pSettings);
      GetObjectManager()->DestroyObject(pSettings);
      XII_VERIFY(accessor.xiiObjectAccessorBase::AddObject(pRoot, "Settings", xiiVariant(), pSettingsType, id).Succeeded(), "Adding scene settings object to root failed.");
    }
  }
}

const xiiDocumentObject* xiiSceneDocument::GetSettingsObject() const
{
  auto       pRoot = GetObjectManager()->GetRootObject();
  xiiVariant value;
  XII_VERIFY(GetObjectAccessor()->GetValue(pRoot, "Settings", value).Succeeded(), "The scene doc root should have a settings property.");
  xiiUuid id = value.Get<xiiUuid>();
  return GetObjectManager()->GetObject(id);
}

const xiiSceneDocumentSettingsBase* xiiSceneDocument::GetSettingsBase() const
{
  return static_cast<const xiiSceneDocumentSettingsBase*>(m_ObjectMirror.GetNativeObjectPointer(GetSettingsObject()));
}

xiiStatus xiiSceneDocument::CreateExposedProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProperty, xiiVariant index, xiiExposedSceneProperty& out_key) const
{
  const xiiDocumentObject* pNodeComponent = xiiObjectPropertyPath::FindParentNodeComponent(pObject);
  if (!pObject)
    return xiiStatus("No parent node or component found.");

  xiiObjectPropertyPathContext context     = {pNodeComponent, GetObjectAccessor(), "Children"};
  xiiPropertyReference         propertyRef = {pObject->GetGuid(), pProperty, index};
  xiiStringBuilder             sPropertyPath;
  xiiStatus                    res = xiiObjectPropertyPath::CreatePropertyPath(context, propertyRef, sPropertyPath);
  if (res.Failed())
    return res;

  out_key.m_Object        = pNodeComponent->GetGuid();
  out_key.m_sPropertyPath = sPropertyPath;
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiSceneDocument::AddExposedParameter(const char* szName, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProperty, xiiVariant index)
{
  if (m_DocumentType != DocumentType::Prefab)
    return xiiStatus("Exposed parameters are only supported in prefab documents.");

  if (FindExposedParameter(pObject, pProperty, index) != -1)
    return xiiStatus("Exposed parameter already exists.");

  xiiExposedSceneProperty key;
  xiiStatus               res = CreateExposedProperty(pObject, pProperty, index, key);
  if (res.Failed())
    return res;

  xiiUuid id;
  res = GetObjectAccessor()->AddObject(GetSettingsObject(), "ExposedProperties", -1, xiiGetStaticRTTI<xiiExposedSceneProperty>(), id);
  if (res.Failed())
    return res;
  const xiiDocumentObject* pParam = GetObjectManager()->GetObject(id);
  GetObjectAccessor()->SetValue(pParam, "Name", szName).LogFailure();
  GetObjectAccessor()->SetValue(pParam, "Object", key.m_Object).LogFailure();
  GetObjectAccessor()->SetValue(pParam, "PropertyPath", xiiVariant(key.m_sPropertyPath)).LogFailure();
  return xiiStatus(XII_SUCCESS);
}

xiiInt32 xiiSceneDocument::FindExposedParameter(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProperty, xiiVariant index)
{
  XII_ASSERT_DEV(m_DocumentType == DocumentType::Prefab, "Exposed properties are only supported in prefab documents.");

  xiiExposedSceneProperty key;
  xiiStatus               res = CreateExposedProperty(pObject, pProperty, index, key);
  if (res.Failed())
    return -1;

  const xiiPrefabDocumentSettings* settings = GetSettings<xiiPrefabDocumentSettings>();
  for (xiiUInt32 i = 0; i < settings->m_ExposedProperties.GetCount(); i++)
  {
    const auto& param = settings->m_ExposedProperties[i];
    if (param.m_Object == key.m_Object && param.m_sPropertyPath == key.m_sPropertyPath)
      return (xiiInt32)i;
  }
  return -1;
}

xiiStatus xiiSceneDocument::RemoveExposedParameter(xiiInt32 index)
{
  xiiVariant value;
  auto       res = GetObjectAccessor()->GetValue(GetSettingsObject(), "ExposedProperties", value, index);
  if (res.Failed())
    return res;

  xiiUuid id = value.Get<xiiUuid>();
  return GetObjectAccessor()->RemoveObject(GetObjectManager()->GetObject(id));
}


void xiiSceneDocument::StoreFavoriteCamera(xiiUInt8 uiSlot)
{
  XII_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  xiiQuadViewPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiQuadViewPreferencesUser>(this);
  auto&                       cam          = pPreferences->m_FavoriteCamera[uiSlot];

  auto* pView = xiiQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView)
  {
    const auto& camera = pView->m_pViewConfig->m_Camera;

    cam.m_PerspectiveMode = pView->m_pViewConfig->m_Perspective;
    cam.m_vCamPos         = camera.GetCenterPosition();
    cam.m_vCamDir         = camera.GetCenterDirForwards();
    cam.m_vCamUp          = camera.GetCenterDirUp();

    // make sure the data gets saved
    pPreferences->TriggerPreferencesChangedEvent();
  }
}

void xiiSceneDocument::RestoreFavoriteCamera(xiiUInt8 uiSlot)
{
  XII_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  xiiQuadViewPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiQuadViewPreferencesUser>(this);
  auto&                       cam          = pPreferences->m_FavoriteCamera[uiSlot];

  auto* pView = xiiQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView == nullptr)
    return;

  xiiVec3 vCamPos = cam.m_vCamPos;
  xiiVec3 vCamDir = cam.m_vCamDir;
  xiiVec3 vCamUp  = cam.m_vCamUp;

  // if the projection mode of the view is orthographic, ignore the direction of the stored favorite camera
  // if we apply a favorite that was saved in an orthographic view, and we apply it to a perspective view,
  // we want to ignore one of the axis, as the respective orthographic position can be arbitrary
  switch (pView->m_pViewConfig->m_Perspective)
  {
    case xiiSceneViewPerspective::Orthogonal_Front:
    case xiiSceneViewPerspective::Orthogonal_Right:
    case xiiSceneViewPerspective::Orthogonal_Top:
      vCamDir = pView->m_pViewConfig->m_Camera.GetCenterDirForwards();
      vCamUp  = pView->m_pViewConfig->m_Camera.GetCenterDirUp();
      break;

    case xiiSceneViewPerspective::Perspective:
    {
      const xiiVec3 vOldPos = pView->m_pViewConfig->m_Camera.GetCenterPosition();

      switch (cam.m_PerspectiveMode)
      {
        case xiiSceneViewPerspective::Orthogonal_Front:
          vCamPos.x = vOldPos.x;
          break;
        case xiiSceneViewPerspective::Orthogonal_Right:
          vCamPos.y = vOldPos.y;
          break;
        case xiiSceneViewPerspective::Orthogonal_Top:
          vCamPos.z = vOldPos.z;
          break;
        case xiiSceneViewPerspective::Perspective:
          break;
      }

      break;
    }
  }

  pView->InterpolateCameraTo(vCamPos, vCamDir, pView->m_pViewConfig->m_Camera.GetFovOrDim(), &vCamUp);
}

xiiResult xiiSceneDocument::JumpToLevelCamera(xiiUInt8 uiSlot, bool bImmediate)
{
  XII_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  auto* pView = xiiQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView == nullptr)
    return XII_FAILURE;

  auto* pObjMan = GetObjectManager();

  xiiHybridArray<xiiDocumentObject*, 8> stack;
  stack.PushBack(pObjMan->GetRootObject());

  const xiiRTTI*           pCamType = xiiGetStaticRTTI<xiiCameraComponent>();
  const xiiDocumentObject* pCamObj  = nullptr;

  while (!stack.IsEmpty())
  {
    const xiiDocumentObject* pObj = stack.PeekBack();
    stack.PopBack();

    stack.PushBackRange(pObj->GetChildren());

    if (pObj->GetType() == pCamType)
    {
      xiiInt32 iShortcut = pObj->GetTypeAccessor().GetValue("EditorShortcut").ConvertTo<xiiInt32>();

      if (iShortcut == uiSlot)
      {
        pCamObj = pObj->GetParent();
        break;
      }
    }
  }

  if (pCamObj == nullptr)
    return XII_FAILURE;

  const xiiTransform tCam = GetGlobalTransform(pCamObj);

  xiiVec3 vCamDir = tCam.m_qRotation * xiiVec3(1, 0, 0);
  xiiVec3 vCamUp  = tCam.m_qRotation * xiiVec3(0, 0, 1);

  // if the projection mode of the view is orthographic, ignore the direction of the level camera
  switch (pView->m_pViewConfig->m_Perspective)
  {
    case xiiSceneViewPerspective::Orthogonal_Front:
    case xiiSceneViewPerspective::Orthogonal_Right:
    case xiiSceneViewPerspective::Orthogonal_Top:
      vCamDir = pView->m_pViewConfig->m_Camera.GetCenterDirForwards();
      vCamUp  = pView->m_pViewConfig->m_Camera.GetCenterDirUp();
      break;

    case xiiSceneViewPerspective::Perspective:
      break;
  }

  pView->InterpolateCameraTo(tCam.m_vPosition, vCamDir, pView->m_pViewConfig->m_Camera.GetFovOrDim(), &vCamUp, bImmediate);

  return XII_SUCCESS;
}

xiiResult xiiSceneDocument::CreateLevelCamera(xiiUInt8 uiSlot)
{
  XII_ASSERT_DEBUG(uiSlot < 10, "Invalid slot");

  auto* pView = xiiQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;

  if (pView == nullptr)
    return XII_FAILURE;

  if (pView->m_pViewConfig->m_Perspective != xiiSceneViewPerspective::Perspective)
    return XII_FAILURE;

  const auto* pRootObj = GetObjectManager()->GetRootObject();

  const xiiVec3 vPos = pView->m_pViewConfig->m_Camera.GetCenterPosition();
  const xiiVec3 vDir = pView->m_pViewConfig->m_Camera.GetCenterDirForwards().GetNormalized();
  const xiiVec3 vUp  = pView->m_pViewConfig->m_Camera.GetCenterDirUp().GetNormalized();

  auto* pAccessor = GetObjectAccessor();
  pAccessor->StartTransaction("Create Level Camera");

  xiiUuid camObjGuid;
  if (pAccessor->AddObject(pRootObj, "Children", -1, xiiGetStaticRTTI<xiiGameObject>(), camObjGuid).Failed())
  {
    pAccessor->CancelTransaction();
    return XII_FAILURE;
  }

  xiiMat3 mRot;
  mRot.SetColumn(0, vDir);
  mRot.SetColumn(1, vUp.CrossRH(vDir).GetNormalized());
  mRot.SetColumn(2, vUp);
  xiiQuat qRot;
  qRot.SetFromMat3(mRot);
  qRot.Normalize();

  SetGlobalTransform(pAccessor->GetObject(camObjGuid), xiiTransform(vPos, qRot), TransformationChanges::Translation | TransformationChanges::Rotation);

  xiiUuid camCompGuid;
  if (pAccessor->AddObject(pAccessor->GetObject(camObjGuid), "Components", -1, xiiGetStaticRTTI<xiiCameraComponent>(), camCompGuid).Failed())
  {
    pAccessor->CancelTransaction();
    return XII_FAILURE;
  }

  if (pAccessor->SetValue(pAccessor->GetObject(camCompGuid), "EditorShortcut", uiSlot).Failed())
  {
    pAccessor->CancelTransaction();
    return XII_FAILURE;
  }

  pAccessor->FinishTransaction();
  return XII_SUCCESS;
}

void xiiSceneDocument::DocumentObjectMetaDataEventHandler(const xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & xiiDocumentObjectMetaData::HiddenFlag) != 0)
  {
    xiiObjectTagMsgToEngine msg;
    msg.m_bSetTag             = e.m_pValue->m_bHidden;
    msg.m_sTag                = "EditorHidden";
    msg.m_bApplyOnAllChildren = true;

    SendObjectMsg(GetObjectManager()->GetObject(e.m_ObjectKey), &msg);
  }
}

void xiiSceneDocument::EngineConnectionEventHandler(const xiiEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case xiiEditorEngineProcessConnection::Event::Type::ProcessCrashed:
    case xiiEditorEngineProcessConnection::Event::Type::ProcessShutdown:
    case xiiEditorEngineProcessConnection::Event::Type::ProcessStarted:
      SetGameMode(GameMode::Off);
      break;

    default:
      break;
  }
}


void xiiSceneDocument::ToolsProjectEventHandler(const xiiToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiToolsProjectEvent::Type::ProjectConfigChanged:
    {
      // we are lazy and just re-select everything here
      // that ensures that ui elements will rebuild their content

      xiiDeque<const xiiDocumentObject*> selection = GetSelectionManager()->GetSelection();
      GetSelectionManager()->SetSelection(selection);
    }
    break;

    default:
      break;
  }
}

void xiiSceneDocument::HandleGameModeMsg(const xiiGameModeMsgToEditor* pMsg)
{
  if (m_GameMode == GameMode::Simulate)
  {
    if (pMsg->m_bRunningPTG)
    {
      m_GameMode = GameMode::Off;
      xiiLog::Warning("Incorrect state change from 'simulate' to 'play-the-game'");
    }
    else
    {
      // probably the message just arrived late ?
      return;
    }
  }

  if (m_GameMode == GameMode::Off || m_GameMode == GameMode::Play)
  {
    SetGameMode(pMsg->m_bRunningPTG ? GameMode::Play : GameMode::Off);
    return;
  }

  XII_REPORT_FAILURE("Unreachable Code reached.");
}


void xiiSceneDocument::HandleVisualScriptActivityMsg(const xiiVisualScriptActivityMsgToEditor* pMsg)
{
  BroadcastInterDocumentMessage(const_cast<xiiVisualScriptActivityMsgToEditor*>(pMsg), this);
}

void xiiSceneDocument::HandleObjectStateFromEngineMsg(const xiiPushObjectStateMsgToEditor* pMsg)
{
  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Pull Object State");

  for (const auto& state : pMsg->m_ObjectStates)
  {
    auto pObject = GetObjectManager()->GetObject(state.m_ObjectGuid);

    if (pObject)
    {
      SetGlobalTransform(pObject, xiiTransform(state.m_vPosition, state.m_qRotation), TransformationChanges::Translation | TransformationChanges::Rotation);
    }
  }

  pHistory->FinishTransaction();
}

void xiiSceneDocument::SendObjectMsg(const xiiDocumentObject* pObj, xiiObjectTagMsgToEngine* pMsg)
{
  // if xiiObjectTagMsgToEngine were derived from a general 'object msg' one could send other message types as well

  if (pObj == nullptr || !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
    return;

  pMsg->m_ObjectGuid = pObj->GetGuid();
  GetEditorEngineConnection()->SendMessage(pMsg);
}
void xiiSceneDocument::SendObjectMsgRecursive(const xiiDocumentObject* pObj, xiiObjectTagMsgToEngine* pMsg)
{
  // if xiiObjectTagMsgToEngine were derived from a general 'object msg' one could send other message types as well

  if (pObj == nullptr || !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
    return;

  pMsg->m_ObjectGuid = pObj->GetGuid();
  GetEditorEngineConnection()->SendMessage(pMsg);

  for (auto pChild : pObj->GetChildren())
  {
    SendObjectMsgRecursive(pChild, pMsg);
  }
}

void xiiSceneDocument::GatherObjectsOfType(xiiDocumentObject* pRoot, xiiGatherObjectsOfTypeMsgInterDoc* pMsg) const
{
  if (pRoot->GetType() == pMsg->m_pType)
  {
    xiiStringBuilder sFullPath;
    GenerateFullDisplayName(pRoot, sFullPath);

    auto& res          = pMsg->m_Results.ExpandAndGetRef();
    res.m_ObjectGuid   = pRoot->GetGuid();
    res.m_pDocument    = this;
    res.m_sDisplayName = sFullPath;
  }

  for (auto pChild : pRoot->GetChildren())
  {
    GatherObjectsOfType(pChild, pMsg);
  }
}

void xiiSceneDocument::OnInterDocumentMessage(xiiReflectedClass* pMessage, xiiDocument* pSender)
{
  // #TODO needs to be overwritten by Scene2
  if (pMessage->GetDynamicRTTI()->IsDerivedFrom<xiiGatherObjectsOfTypeMsgInterDoc>())
  {
    GatherObjectsOfType(GetObjectManager()->GetRootObject(), static_cast<xiiGatherObjectsOfTypeMsgInterDoc*>(pMessage));
  }
}

xiiStatus xiiSceneDocument::RequestExportScene(const char* szTargetFile, const xiiAssetFileHeader& header)
{
  if (GetGameMode() != GameMode::Off)
    return xiiStatus("Cannot export while the scene is simulating");

  XII_SUCCEED_OR_RETURN(SaveDocument());

  const xiiStatus status = xiiAssetDocument::RemoteExport(header, szTargetFile);

  // make sure the world is reset
  SendGameWorldToEngine();

  return status;
}

void xiiSceneDocument::UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  // scenes do not have exposed parameters
  if (!IsPrefab())
    return;

  xiiExposedParameters* pExposedParams = XII_DEFAULT_NEW(xiiExposedParameters);

  if (m_DocumentType == DocumentType::Prefab)
  {
    xiiSet<xiiString> alreadyExposed;

    auto pSettings = GetSettings<xiiPrefabDocumentSettings>();
    for (auto prop : pSettings->m_ExposedProperties)
    {
      auto pRootObject = GetObjectManager()->GetObject(prop.m_Object);
      if (!pRootObject)
      {
        xiiLog::Warning("The exposed scene property '{0}' does not point to a valid object and is skipped.", prop.m_sName);
        continue;
      }

      xiiObjectPropertyPathContext context = {pRootObject, GetObjectAccessor(), "Children"};

      xiiPropertyReference key;
      auto                 res = xiiObjectPropertyPath::ResolvePropertyPath(context, prop.m_sPropertyPath, key);
      if (res.Failed())
      {
        xiiLog::Warning("The exposed scene property '{0}' can no longer be resolved and is skipped.", prop.m_sName);
        continue;
      }
      xiiVariant value;

      auto pLeafObject = GetObjectManager()->GetObject(key.m_Object);
      if (const xiiExposedParametersAttribute* pAttrib = key.m_pProperty->GetAttributeByType<xiiExposedParametersAttribute>())
      {
        const xiiAbstractProperty* pParameterSourceProp = pLeafObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
        XII_ASSERT_DEBUG(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(), pLeafObject->GetType()->GetTypeName());

        xiiExposedParameterCommandAccessor proxy(context.m_pAccessor, key.m_pProperty, pParameterSourceProp);
        res = proxy.GetValue(pLeafObject, key.m_pProperty, value, key.m_Index);
      }
      else
      {
        res = context.m_pAccessor->GetValue(pLeafObject, key.m_pProperty, value, key.m_Index);
      }
      XII_ASSERT_DEBUG(res.Succeeded(), "ResolvePropertyPath succeeded so GetValue should too");

      // do not show the same parameter twice, even if they have different types, as the UI doesn't handle that case properly
      // TODO: we should prevent users from using the same name for differently typed parameters
      // don't do this earlier, we do want to validate each exposed property with the code above
      if (alreadyExposed.Contains(prop.m_sName))
        continue;

      alreadyExposed.Insert(prop.m_sName);

      xiiExposedParameter* param = XII_DEFAULT_NEW(xiiExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName        = prop.m_sName;
      param->m_sType        = key.m_pProperty->GetSpecificType()->GetTypeName();
      param->m_DefaultValue = value;
      for (auto attrib : key.m_pProperty->GetAttributes())
      {
        param->m_Attributes.PushBack(xiiReflectionSerializer::Clone(attrib));
      }
    }
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

xiiTransformStatus xiiSceneDocument::ExportScene(bool bCreateThumbnail)
{
  if (GetUnknownObjectTypeInstances() > 0)
  {
    return xiiTransformStatus("Can't export scene/prefab when it contains unknown object types.");
  }

  // \todo Add export support for layers
  auto saveres = SaveDocument();

  if (saveres.m_Result.Failed())
    return saveres;

  xiiTransformStatus res;

  if (bCreateThumbnail)
  {
    // this is needed to generate a scene thumbnail, however that has a larger overhead (1 sec or so)
    res = xiiAssetCurator::GetSingleton()->TransformAsset(GetGuid(), xiiTransformFlags::ForceTransform | xiiTransformFlags::TriggeredManually);
  }
  else
    res = TransformAsset(xiiTransformFlags::ForceTransform | xiiTransformFlags::TriggeredManually);

  if (res.Failed())
    xiiLog::Error(res.m_sMessage);
  else
    xiiLog::Success(res.m_sMessage);

  ShowDocumentStatus(res.m_sMessage.GetData());

  return res;
}

void xiiSceneDocument::ExportSceneGeometry(const char* szFile, bool bOnlySelection, int iExtractionMode, const xiiMat3& mTransform)
{
  xiiExportSceneGeometryMsgToEngine msg;
  msg.m_sOutputFile     = szFile;
  msg.m_bSelectionOnly  = bOnlySelection;
  msg.m_iExtractionMode = iExtractionMode;
  msg.m_Transform       = mTransform;

  SendMessageToEngine(&msg);

  xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(xiiFmt("Geometry exported to '{0}'", szFile), xiiTime::Seconds(5.0f));
}

void xiiSceneDocument::HandleEngineMessage(const xiiEditorEngineDocumentMsg* pMsg)
{
  xiiGameObjectDocument::HandleEngineMessage(pMsg);

  if (const xiiGameModeMsgToEditor* msg = xiiDynamicCast<const xiiGameModeMsgToEditor*>(pMsg))
  {
    HandleGameModeMsg(msg);
    return;
  }

  if (const xiiVisualScriptActivityMsgToEditor* msg = xiiDynamicCast<const xiiVisualScriptActivityMsgToEditor*>(pMsg))
  {
    HandleVisualScriptActivityMsg(msg);
    return;
  }

  if (const xiiDocumentOpenResponseMsgToEditor* msg = xiiDynamicCast<const xiiDocumentOpenResponseMsgToEditor*>(pMsg))
  {
    SyncObjectHiddenState();
  }

  if (const xiiPushObjectStateMsgToEditor* msg = xiiDynamicCast<const xiiPushObjectStateMsgToEditor*>(pMsg))
  {
    HandleObjectStateFromEngineMsg(msg);
  }
}

xiiTransformStatus xiiSceneDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  if (m_DocumentType == DocumentType::Prefab)
  {
    const xiiPrefabDocumentSettings* pSettings = GetSettings<xiiPrefabDocumentSettings>();

    if (GetEditorEngineConnection() != nullptr)
    {
      xiiExposedDocumentObjectPropertiesMsgToEngine msg;
      msg.m_Properties = pSettings->m_ExposedProperties;

      SendMessageToEngine(&msg);
    }
  }
  return RequestExportScene(szTargetFile, AssetHeader);
}


xiiTransformStatus xiiSceneDocument::InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags)
{
  XII_ASSERT_NOT_IMPLEMENTED;

  /* this function is never called */
  return xiiStatus(XII_FAILURE);
}


xiiTransformStatus xiiSceneDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  xiiStatus status = xiiAssetDocument::RemoteCreateThumbnail(ThumbnailInfo, {});

  // if we were to do this BEFORE making the screenshot, the scene would be killed, but not immediately restored
  // and the screenshot would end up empty
  // instead the engine side ensures simulation is stopped and makes a screenshot of whatever state is visible
  // but then the editor and engine state are out of sync, so AFTER the screenshot is done,
  // we ensure to also stop simulation on the editor side
  StopGameMode();

  return status;
}

void xiiSceneDocument::SyncObjectHiddenState()
{
  // #TODO Scene2 handling
  for (auto pChild : GetObjectManager()->GetRootObject()->GetChildren())
  {
    SyncObjectHiddenState(pChild);
  }
}

void xiiSceneDocument::SyncObjectHiddenState(xiiDocumentObject* pObject)
{
  const bool bHidden = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid())->m_bHidden;
  m_DocumentObjectMetaData->EndReadMetaData();

  xiiObjectTagMsgToEngine msg;
  msg.m_bSetTag = bHidden;
  msg.m_sTag    = "EditorHidden";

  SendObjectMsg(pObject, &msg);

  for (auto pChild : pObject->GetChildren())
  {
    SyncObjectHiddenState(pChild);
  }
}

void xiiSceneDocument::UpdateObjectDebugTargets()
{
  xiiGatherObjectsForDebugVisMsgInterDoc msg;
  BroadcastInterDocumentMessage(&msg, this);

  {
    xiiObjectsForDebugVisMsgToEngine msgToEngine;
    msgToEngine.m_Objects.SetCountUninitialized(sizeof(xiiUuid) * msg.m_Objects.GetCount());

    xiiMemoryUtils::Copy<xiiUInt8>(msgToEngine.m_Objects.GetData(), reinterpret_cast<xiiUInt8*>(msg.m_Objects.GetData()), msgToEngine.m_Objects.GetCount());

    GetEditorEngineConnection()->SendMessage(&msgToEngine);
  }
}
