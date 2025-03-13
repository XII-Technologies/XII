#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Actions/GameObjectContextActions.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Document/GameObjectContextDocument.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameObjectContextAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActionDescriptorHandle xiiGameObjectContextActions::s_hCategory;
xiiActionDescriptorHandle xiiGameObjectContextActions::s_hPickContextScene;
xiiActionDescriptorHandle xiiGameObjectContextActions::s_hPickContextObject;
xiiActionDescriptorHandle xiiGameObjectContextActions::s_hClearContextObject;

void xiiGameObjectContextActions::RegisterActions()
{
  s_hCategory           = XII_REGISTER_CATEGORY("GameObjectContextCategory");
  s_hPickContextScene   = XII_REGISTER_ACTION_1("GameObjectContext.PickContextScene", xiiActionScope::Window, "Game Object Context", "", xiiGameObjectContextAction, xiiGameObjectContextAction::ActionType::PickContextScene);
  s_hPickContextObject  = XII_REGISTER_ACTION_1("GameObjectContext.PickContextObject", xiiActionScope::Window, "Game Object Context", "", xiiGameObjectContextAction, xiiGameObjectContextAction::ActionType::PickContextObject);
  s_hClearContextObject = XII_REGISTER_ACTION_1("GameObjectContext.ClearContextObject", xiiActionScope::Window, "Game Object Context", "", xiiGameObjectContextAction, xiiGameObjectContextAction::ActionType::ClearContextObject);
}

void xiiGameObjectContextActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hCategory);
  xiiActionManager::UnregisterAction(s_hPickContextScene);
  xiiActionManager::UnregisterAction(s_hPickContextObject);
  xiiActionManager::UnregisterAction(s_hClearContextObject);
}

void xiiGameObjectContextActions::MapToolbarActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 10.0f);

  const xiiStringView szSubPath = "GameObjectContextCategory";
  pMap->MapAction(s_hPickContextScene, szSubPath, 1.0f);
}

void xiiGameObjectContextActions::MapContextMenuActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 10.0f);

  const xiiStringView szSubPath = "GameObjectContextCategory";
  pMap->MapAction(s_hPickContextObject, szSubPath, 1.0f);
  pMap->MapAction(s_hClearContextObject, szSubPath, 2.0f);
}

xiiGameObjectContextAction::xiiGameObjectContextAction(const xiiActionContext& context, const char* szName, xiiGameObjectContextAction::ActionType type) :
  xiiButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::PickContextScene:
      SetIconPath(":/EditorPluginAssets/PickTarget.svg");
      break;
    case ActionType::PickContextObject:
      SetIconPath(":/EditorPluginAssets/PickTarget.svg");
      break;
    case ActionType::ClearContextObject:
      SetIconPath(":/EditorPluginAssets/PickTarget.svg");
      break;
    default:
      break;
  }

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiGameObjectContextAction::SelectionEventHandler, this));
  Update();
}

xiiGameObjectContextAction::~xiiGameObjectContextAction()
{
  m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiGameObjectContextAction::SelectionEventHandler, this));
}

void xiiGameObjectContextAction::Execute(const xiiVariant& value)
{
  xiiGameObjectContextDocument* pDocument = static_cast<xiiGameObjectContextDocument*>(GetContext().m_pDocument);
  xiiUuid                       document  = pDocument->GetContextDocumentGuid();
  switch (m_Type)
  {
    case ActionType::PickContextScene:
    {
      xiiQtAssetBrowserDlg dlg(GetContext().m_pWindow, document, "Scene;Prefab");
      if (dlg.exec() == 0)
        return;

      document = dlg.GetSelectedAssetGuid();
      pDocument->SetContext(document, xiiUuid()).LogFailure();
      return;
    }
    case ActionType::PickContextObject:
    {
      const auto& selection = pDocument->GetSelectionManager()->GetSelection();
      if (selection.GetCount() == 1)
      {
        if (selection[0]->GetType() == xiiGetStaticRTTI<xiiGameObject>())
        {
          pDocument->SetContext(document, selection[0]->GetGuid()).LogFailure();
        }
      }
      return;
    }
    case ActionType::ClearContextObject:
    {
      pDocument->SetContext(document, xiiUuid()).LogFailure();
      return;
    }
  }
}

void xiiGameObjectContextAction::SelectionEventHandler(const xiiSelectionManagerEvent& e)
{
  Update();
}

void xiiGameObjectContextAction::Update()
{
  xiiGameObjectContextDocument* pDocument = static_cast<xiiGameObjectContextDocument*>(GetContext().m_pDocument);

  switch (m_Type)
  {
    case ActionType::PickContextObject:
    {
      const auto& selection           = pDocument->GetSelectionManager()->GetSelection();
      bool        bIsSingleGameObject = selection.GetCount() == 1 && selection[0]->GetType() == xiiGetStaticRTTI<xiiGameObject>();
      SetEnabled(bIsSingleGameObject);
    }
    break;

    default:
      break;
  }
}
