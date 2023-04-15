#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <QFileDialog>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSelectionAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiActionDescriptorHandle xiiSelectionActions::s_hGroupSelectedItems;
xiiActionDescriptorHandle xiiSelectionActions::s_hCreateEmptyChildObject;
xiiActionDescriptorHandle xiiSelectionActions::s_hCreateEmptyObjectAtPosition;
xiiActionDescriptorHandle xiiSelectionActions::s_hHideSelectedObjects;
xiiActionDescriptorHandle xiiSelectionActions::s_hHideUnselectedObjects;
xiiActionDescriptorHandle xiiSelectionActions::s_hShowHiddenObjects;
xiiActionDescriptorHandle xiiSelectionActions::s_hPrefabMenu;
xiiActionDescriptorHandle xiiSelectionActions::s_hCreatePrefab;
xiiActionDescriptorHandle xiiSelectionActions::s_hRevertPrefab;
xiiActionDescriptorHandle xiiSelectionActions::s_hUnlinkFromPrefab;
xiiActionDescriptorHandle xiiSelectionActions::s_hOpenPrefabDocument;
xiiActionDescriptorHandle xiiSelectionActions::s_hDuplicateSpecial;
xiiActionDescriptorHandle xiiSelectionActions::s_hDeltaTransform;
xiiActionDescriptorHandle xiiSelectionActions::s_hSnapObjectToCamera;
xiiActionDescriptorHandle xiiSelectionActions::s_hAttachToObject;
xiiActionDescriptorHandle xiiSelectionActions::s_hDetachFromParent;
xiiActionDescriptorHandle xiiSelectionActions::s_hConvertToEnginePrefab;
xiiActionDescriptorHandle xiiSelectionActions::s_hConvertToEditorPrefab;
xiiActionDescriptorHandle xiiSelectionActions::s_hCopyReference;



void xiiSelectionActions::RegisterActions()
{
  s_hGroupSelectedItems          = XII_REGISTER_ACTION_1("Selection.GroupItems", xiiActionScope::Document, "Scene - Selection", "Ctrl+G", xiiSelectionAction, xiiSelectionAction::ActionType::GroupSelectedItems);
  s_hCreateEmptyChildObject      = XII_REGISTER_ACTION_1("Selection.CreateEmptyChildObject", xiiActionScope::Document, "Scene - Selection", "", xiiSelectionAction, xiiSelectionAction::ActionType::CreateEmptyChildObject);
  s_hCreateEmptyObjectAtPosition = XII_REGISTER_ACTION_1("Selection.CreateEmptyObjectAtPosition", xiiActionScope::Document, "Scene - Selection", "Ctrl+Shift+X", xiiSelectionAction, xiiSelectionAction::ActionType::CreateEmptyObjectAtPosition);
  s_hHideSelectedObjects         = XII_REGISTER_ACTION_1("Selection.HideItems", xiiActionScope::Document, "Scene - Selection", "H", xiiSelectionAction, xiiSelectionAction::ActionType::HideSelectedObjects);
  s_hHideUnselectedObjects       = XII_REGISTER_ACTION_1("Selection.HideUnselectedItems", xiiActionScope::Document, "Scene - Selection", "Shift+H", xiiSelectionAction, xiiSelectionAction::ActionType::HideUnselectedObjects);
  s_hShowHiddenObjects           = XII_REGISTER_ACTION_1("Selection.ShowHidden", xiiActionScope::Document, "Scene - Selection", "Ctrl+H", xiiSelectionAction, xiiSelectionAction::ActionType::ShowHiddenObjects);
  s_hAttachToObject              = XII_REGISTER_ACTION_1("Selection.Attach", xiiActionScope::Document, "Scene - Selection", "", xiiSelectionAction, xiiSelectionAction::ActionType::AttachToObject);
  s_hDetachFromParent            = XII_REGISTER_ACTION_1("Selection.Detach", xiiActionScope::Document, "Scene - Selection", "", xiiSelectionAction, xiiSelectionAction::ActionType::DetachFromParent);

  s_hPrefabMenu            = XII_REGISTER_MENU_WITH_ICON("Prefabs.Menu", ":/AssetIcons/Prefab.png");
  s_hCreatePrefab          = XII_REGISTER_ACTION_1("Prefabs.Create", xiiActionScope::Document, "Prefabs", "", xiiSelectionAction, xiiSelectionAction::ActionType::CreatePrefab);
  s_hRevertPrefab          = XII_REGISTER_ACTION_1("Prefabs.Revert", xiiActionScope::Document, "Prefabs", "", xiiSelectionAction, xiiSelectionAction::ActionType::RevertPrefab);
  s_hUnlinkFromPrefab      = XII_REGISTER_ACTION_1("Prefabs.Unlink", xiiActionScope::Document, "Prefabs", "", xiiSelectionAction, xiiSelectionAction::ActionType::UnlinkFromPrefab);
  s_hOpenPrefabDocument    = XII_REGISTER_ACTION_1("Prefabs.OpenDocument", xiiActionScope::Document, "Prefabs", "", xiiSelectionAction, xiiSelectionAction::ActionType::OpenPrefabDocument);
  s_hConvertToEnginePrefab = XII_REGISTER_ACTION_1("Prefabs.ConvertToEngine", xiiActionScope::Document, "Prefabs", "", xiiSelectionAction, xiiSelectionAction::ActionType::ConvertToEnginePrefab);
  s_hConvertToEditorPrefab = XII_REGISTER_ACTION_1("Prefabs.ConvertToEditor", xiiActionScope::Document, "Prefabs", "", xiiSelectionAction, xiiSelectionAction::ActionType::ConvertToEditorPrefab);

  s_hDuplicateSpecial   = XII_REGISTER_ACTION_1("Selection.DuplicateSpecial", xiiActionScope::Document, "Scene - Selection", "Ctrl+D", xiiSelectionAction, xiiSelectionAction::ActionType::DuplicateSpecial);
  s_hDeltaTransform     = XII_REGISTER_ACTION_1("Selection.DeltaTransform", xiiActionScope::Document, "Scene - Selection", "Ctrl+M", xiiSelectionAction, xiiSelectionAction::ActionType::DeltaTransform);
  s_hSnapObjectToCamera = XII_REGISTER_ACTION_1("Scene.Camera.SnapObjectToCamera", xiiActionScope::Document, "Camera", "", xiiSelectionAction, xiiSelectionAction::ActionType::SnapObjectToCamera);
  s_hCopyReference      = XII_REGISTER_ACTION_1("Selection.CopyReference", xiiActionScope::Document, "Scene - Selection", "", xiiSelectionAction, xiiSelectionAction::ActionType::CopyReference);
}

void xiiSelectionActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hGroupSelectedItems);
  xiiActionManager::UnregisterAction(s_hCreateEmptyChildObject);
  xiiActionManager::UnregisterAction(s_hCreateEmptyObjectAtPosition);
  xiiActionManager::UnregisterAction(s_hHideSelectedObjects);
  xiiActionManager::UnregisterAction(s_hHideUnselectedObjects);
  xiiActionManager::UnregisterAction(s_hShowHiddenObjects);
  xiiActionManager::UnregisterAction(s_hPrefabMenu);
  xiiActionManager::UnregisterAction(s_hCreatePrefab);
  xiiActionManager::UnregisterAction(s_hRevertPrefab);
  xiiActionManager::UnregisterAction(s_hUnlinkFromPrefab);
  xiiActionManager::UnregisterAction(s_hOpenPrefabDocument);
  xiiActionManager::UnregisterAction(s_hDuplicateSpecial);
  xiiActionManager::UnregisterAction(s_hDeltaTransform);
  xiiActionManager::UnregisterAction(s_hSnapObjectToCamera);
  xiiActionManager::UnregisterAction(s_hAttachToObject);
  xiiActionManager::UnregisterAction(s_hDetachFromParent);
  xiiActionManager::UnregisterAction(s_hConvertToEditorPrefab);
  xiiActionManager::UnregisterAction(s_hConvertToEnginePrefab);
  xiiActionManager::UnregisterAction(s_hCopyReference);
}

void xiiSelectionActions::MapActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  xiiStringBuilder sSubPath(szPath, "/SelectionCategory");

  pMap->MapAction(s_hCreateEmptyChildObject, sSubPath, 1.0f);
  pMap->MapAction(s_hCreateEmptyObjectAtPosition, sSubPath, 1.1f);
  pMap->MapAction(s_hGroupSelectedItems, sSubPath, 3.7f);
  pMap->MapAction(s_hHideSelectedObjects, sSubPath, 4.0f);
  pMap->MapAction(s_hHideUnselectedObjects, sSubPath, 5.0f);
  pMap->MapAction(s_hShowHiddenObjects, sSubPath, 6.0f);
  pMap->MapAction(s_hDuplicateSpecial, sSubPath, 7.0f);
  pMap->MapAction(s_hDeltaTransform, sSubPath, 7.1f);
  pMap->MapAction(s_hAttachToObject, sSubPath, 7.2f);
  pMap->MapAction(s_hDetachFromParent, sSubPath, 7.3f);
  pMap->MapAction(s_hSnapObjectToCamera, sSubPath, 9.0f);
  pMap->MapAction(s_hCopyReference, sSubPath, 10.0f);

  MapPrefabActions(szMapping, sSubPath, 0.0f);
}

void xiiSelectionActions::MapPrefabActions(const char* szMapping, const char* szPath, float fPriority)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  xiiStringBuilder sPrefabSubPath(szPath, "/Prefabs.Menu");
  pMap->MapAction(s_hPrefabMenu, szPath, fPriority);

  pMap->MapAction(s_hOpenPrefabDocument, sPrefabSubPath, 1.0f);
  pMap->MapAction(s_hRevertPrefab, sPrefabSubPath, 2.0f);
  pMap->MapAction(s_hCreatePrefab, sPrefabSubPath, 3.0f);
  pMap->MapAction(s_hUnlinkFromPrefab, sPrefabSubPath, 4.0f);
  pMap->MapAction(s_hConvertToEditorPrefab, sPrefabSubPath, 5.0f);
  pMap->MapAction(s_hConvertToEnginePrefab, sPrefabSubPath, 6.0f);
}

void xiiSelectionActions::MapContextMenuActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  xiiStringBuilder sSubPath(szPath, "/SelectionCategory");

  pMap->MapAction(s_hCreateEmptyChildObject, sSubPath, 0.5f);
  pMap->MapAction(s_hGroupSelectedItems, sSubPath, 2.0f);
  pMap->MapAction(s_hHideSelectedObjects, sSubPath, 3.0f);
  pMap->MapAction(s_hDetachFromParent, sSubPath, 3.2f);
  pMap->MapAction(s_hCopyReference, sSubPath, 4.0f);

  MapPrefabActions(szMapping, sSubPath, 4.0f);
}


void xiiSelectionActions::MapViewContextMenuActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  xiiStringBuilder sSubPath(szPath, "/SelectionCategory");

  pMap->MapAction(s_hGroupSelectedItems, sSubPath, 2.0f);
  pMap->MapAction(s_hHideSelectedObjects, sSubPath, 3.0f);
  pMap->MapAction(s_hAttachToObject, sSubPath, 3.1f);
  pMap->MapAction(s_hDetachFromParent, sSubPath, 3.2f);
  pMap->MapAction(s_hSnapObjectToCamera, sSubPath, 5.0f);
  pMap->MapAction(s_hCopyReference, sSubPath, 6.0f);

  MapPrefabActions(szMapping, sSubPath, 7.0f);
}

xiiSelectionAction::xiiSelectionAction(const xiiActionContext& context, const char* szName, xiiSelectionAction::ActionType type) :
  xiiButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pSceneDocument = const_cast<xiiSceneDocument*>(static_cast<const xiiSceneDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::GroupSelectedItems:
      SetIconPath(":/EditorPluginScene/Icons/GroupSelection16.png");
      break;
    case ActionType::CreateEmptyChildObject:
      SetIconPath(":/EditorPluginScene/Icons/CreateNode16.png");
      break;
    case ActionType::CreateEmptyObjectAtPosition:
      SetIconPath(":/EditorPluginScene/Icons/CreateNode16.png");
      break;
    case ActionType::HideSelectedObjects:
      SetIconPath(":/EditorPluginScene/Icons/HideSelected16.png");
      break;
    case ActionType::HideUnselectedObjects:
      SetIconPath(":/EditorPluginScene/Icons/HideUnselected16.png");
      break;
    case ActionType::ShowHiddenObjects:
      SetIconPath(":/EditorPluginScene/Icons/ShowHidden16.png");
      break;
    case ActionType::CreatePrefab:
      SetIconPath(":/EditorPluginScene/Icons/PrefabCreate16.png");
      break;
    case ActionType::RevertPrefab:
      SetIconPath(":/EditorPluginScene/Icons/PrefabRevert16.png");
      break;
    case ActionType::UnlinkFromPrefab:
      SetIconPath(":/EditorPluginScene/Icons/PrefabUnlink16.png");
      break;
    case ActionType::OpenPrefabDocument:
      SetIconPath(":/EditorPluginScene/Icons/PrefabOpenDocument16.png");
      break;
    case ActionType::DuplicateSpecial:
      SetIconPath(":/EditorPluginScene/Icons/Duplicate16.png");
      break;
    case ActionType::DeltaTransform:
      // SetIconPath(":/EditorPluginScene/Icons/Duplicate16.png"); // TODO Icon
      break;
    case ActionType::SnapObjectToCamera:
      // SetIconPath(":/EditorPluginScene/Icons/Duplicate16.png"); // TODO Icon
      break;
    case ActionType::AttachToObject:
      // SetIconPath(":/EditorPluginScene/Icons/Duplicate16.png"); // TODO Icon
      break;
    case ActionType::DetachFromParent:
      // SetIconPath(":/EditorPluginScene/Icons/Duplicate16.png"); // TODO Icon
      break;
    case ActionType::ConvertToEditorPrefab:
      // SetIconPath(":/EditorPluginScene/PrefabRevert.png"); // TODO Icon
      break;
    case ActionType::ConvertToEnginePrefab:
      // SetIconPath(":/EditorPluginScene/PrefabRevert.png"); // TODO Icon
      break;
    case ActionType::CopyReference:
      // SetIconPath(":/EditorPluginScene/PrefabRevert.png"); // TODO Icon
      break;
  }

  UpdateEnableState();

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiSelectionAction::SelectionEventHandler, this));
}


xiiSelectionAction::~xiiSelectionAction()
{
  m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiSelectionAction::SelectionEventHandler, this));
}

void xiiSelectionAction::Execute(const xiiVariant& value)
{
  switch (m_Type)
  {
    case ActionType::GroupSelectedItems:
      m_pSceneDocument->GroupSelection();
      return;
    case ActionType::CreateEmptyChildObject:
    {
      auto res = m_pSceneDocument->CreateEmptyObject(true, false);
      xiiQtUiServices::MessageBoxStatus(res, "Object creation failed.");
      return;
    }
    case ActionType::CreateEmptyObjectAtPosition:
    {
      auto res = m_pSceneDocument->CreateEmptyObject(false, true);
      xiiQtUiServices::MessageBoxStatus(res, "Object creation failed.");
      return;
    }
    case ActionType::HideSelectedObjects:
      m_pSceneDocument->ShowOrHideSelectedObjects(xiiSceneDocument::ShowOrHide::Hide);
      m_pSceneDocument->ShowDocumentStatus("Hiding selected objects");
      break;
    case ActionType::HideUnselectedObjects:
      m_pSceneDocument->HideUnselectedObjects();
      m_pSceneDocument->ShowDocumentStatus("Hiding unselected objects");
      break;
    case ActionType::ShowHiddenObjects:
      m_pSceneDocument->ShowOrHideAllObjects(xiiSceneDocument::ShowOrHide::Show);
      m_pSceneDocument->ShowDocumentStatus("Showing hidden objects");
      break;
    case ActionType::CreatePrefab:
      CreatePrefab();
      break;

    case ActionType::RevertPrefab:
    {
      if (xiiQtUiServices::MessageBoxQuestion("Discard all modifications to the selected prefabs and revert to the prefab template state?",
                                              QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
      {
        const xiiDeque<const xiiDocumentObject*> sel = m_pSceneDocument->GetSelectionManager()->GetTopLevelSelection(xiiGetStaticRTTI<xiiGameObject>());
        m_pSceneDocument->RevertPrefabs(sel);
      }
    }
    break;

    case ActionType::UnlinkFromPrefab:
    {
      if (xiiQtUiServices::MessageBoxQuestion("Unlink the selected prefab instances from their templates?",
                                              QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
      {
        const xiiDeque<const xiiDocumentObject*> sel = m_pSceneDocument->GetSelectionManager()->GetTopLevelSelection(xiiGetStaticRTTI<xiiGameObject>());
        m_pSceneDocument->UnlinkPrefabs(sel);
      }
    }
    break;

    case ActionType::OpenPrefabDocument:
      OpenPrefabDocument();
      break;

    case ActionType::DuplicateSpecial:
      m_pSceneDocument->DuplicateSpecial();
      break;

    case ActionType::DeltaTransform:
      m_pSceneDocument->DeltaTransform();
      break;

    case ActionType::SnapObjectToCamera:
      m_pSceneDocument->SnapObjectToCamera();
      break;

    case ActionType::AttachToObject:
      m_pSceneDocument->AttachToObject();
      break;
    case ActionType::DetachFromParent:
      m_pSceneDocument->DetachFromParent();
      break;

    case ActionType::CopyReference:
      m_pSceneDocument->CopyReference();
      break;

    case ActionType::ConvertToEditorPrefab:
    {
      const xiiDeque<const xiiDocumentObject*> sel = m_pSceneDocument->GetSelectionManager()->GetTopLevelSelection(xiiGetStaticRTTI<xiiGameObject>());
      m_pSceneDocument->ConvertToEditorPrefab(sel);
    }
    break;

    case ActionType::ConvertToEnginePrefab:
    {
      if (xiiQtUiServices::MessageBoxQuestion("Discard all modifications to the selected prefabs and convert them to engine prefabs?",
                                              QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes)
      {
        const xiiDeque<const xiiDocumentObject*> sel = m_pSceneDocument->GetSelectionManager()->GetTopLevelSelection(xiiGetStaticRTTI<xiiGameObject>());
        m_pSceneDocument->ConvertToEnginePrefab(sel);
      }
    }
    break;
  }
}


void xiiSelectionAction::OpenPrefabDocument()
{
  const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

  if (sel.GetCount() != 1)
    return;

  const xiiSceneDocument* pScene = static_cast<const xiiSceneDocument*>(m_Context.m_pDocument);


  xiiUuid PrefabAsset;
  if (pScene->IsObjectEnginePrefab(sel[0]->GetGuid(), &PrefabAsset))
  {
    // PrefabAsset is all we need
  }
  else
  {
    auto pMeta  = pScene->m_DocumentObjectMetaData->BeginReadMetaData(sel[0]->GetGuid());
    PrefabAsset = pMeta->m_CreateFromPrefab;
    pScene->m_DocumentObjectMetaData->EndReadMetaData();
  }

  auto pAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(PrefabAsset);
  if (pAsset)
  {
    xiiQtEditorApp::GetSingleton()->OpenDocumentQueued(pAsset->m_pAssetInfo->m_sAbsolutePath);
  }
  else
  {
    xiiQtUiServices::MessageBoxWarning("The prefab asset of this instance is currently unknown. It may have been deleted. Try updating the "
                                       "asset library ('Check FileSystem'), if it should be there.");
  }
}

void xiiSelectionAction::CreatePrefab()
{
  static xiiString sSearchDir = xiiToolsProject::GetSingleton()->GetProjectFile();

  xiiStringBuilder sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Prefab"),
                                                        QString::fromUtf8(sSearchDir.GetData()), QString::fromUtf8("*.xiiPrefab"), nullptr, QFileDialog::Option::DontResolveSymlinks)
                             .toUtf8()
                             .data();

  if (!sFile.IsEmpty())
  {
    sFile.ChangeFileExtension("xiiPrefab");

    sSearchDir = sFile.GetFileDirectory();

    if (xiiOSFile::ExistsFile(sFile))
    {
      xiiQtUiServices::MessageBoxInformation("You currently cannot replace an existing prefab this way. Please choose a new prefab file.");
      return;
    }

    auto res = m_pSceneDocument->CreatePrefabDocumentFromSelection(sFile, xiiGetStaticRTTI<xiiGameObject>());
    m_pSceneDocument->ScheduleSendObjectSelection(); // fix selection of prefab object
    xiiQtUiServices::MessageBoxStatus(res, "Failed to create Prefab", "Successfully created Prefab");
  }
}

void xiiSelectionAction::SelectionEventHandler(const xiiSelectionManagerEvent& e)
{
  UpdateEnableState();
}

void xiiSelectionAction::UpdateEnableState()
{
  if (m_Type == ActionType::HideSelectedObjects || m_Type == ActionType::DuplicateSpecial || m_Type == ActionType::DeltaTransform ||
      m_Type == ActionType::SnapObjectToCamera || m_Type == ActionType::DetachFromParent || m_Type == ActionType::HideUnselectedObjects ||
      m_Type == ActionType::AttachToObject)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }

  if (m_Type == ActionType::GroupSelectedItems)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() > 1);
  }

  if (m_Type == ActionType::CreateEmptyChildObject)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() <= 1);
  }

  if (m_Type == ActionType::CopyReference)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() == 1);
  }

  if (m_Type == ActionType::OpenPrefabDocument)
  {
    const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

    if (sel.GetCount() != 1)
    {
      SetEnabled(false);
      return;
    }

    const xiiSceneDocument* pScene    = static_cast<const xiiSceneDocument*>(m_Context.m_pDocument);
    const bool              bIsPrefab = pScene->IsObjectEditorPrefab(sel[0]->GetGuid()) || pScene->IsObjectEnginePrefab(sel[0]->GetGuid());

    SetEnabled(bIsPrefab);
    return;
  }

  if (m_Type == ActionType::RevertPrefab || m_Type == ActionType::UnlinkFromPrefab || m_Type == ActionType::ConvertToEnginePrefab ||
      m_Type == ActionType::CreatePrefab)
  {
    const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

    if (sel.IsEmpty())
    {
      SetEnabled(false);
      return;
    }

    if (m_Type == ActionType::CreatePrefab)
    {
      SetEnabled(true);
      return;
    }

    const bool bShouldBePrefab =
      (m_Type == ActionType::RevertPrefab) || (m_Type == ActionType::ConvertToEnginePrefab) || (m_Type == ActionType::UnlinkFromPrefab);

    const xiiSceneDocument* pScene    = static_cast<const xiiSceneDocument*>(m_Context.m_pDocument);
    const bool              bIsPrefab = pScene->IsObjectEditorPrefab(sel[0]->GetGuid());

    SetEnabled(bIsPrefab == bShouldBePrefab);
  }

  if (m_Type == ActionType::ConvertToEditorPrefab)
  {
    const auto& sel = m_Context.m_pDocument->GetSelectionManager()->GetSelection();

    if (sel.IsEmpty())
    {
      SetEnabled(false);
      return;
    }

    const xiiSceneDocument* pScene    = static_cast<const xiiSceneDocument*>(m_Context.m_pDocument);
    const bool              bIsPrefab = pScene->IsObjectEnginePrefab(sel[0]->GetGuid());

    SetEnabled(bIsPrefab);
  }
}
