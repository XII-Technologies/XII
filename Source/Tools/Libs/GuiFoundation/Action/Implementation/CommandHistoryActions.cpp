#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCommandHistoryAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActionDescriptorHandle xiiCommandHistoryActions::s_hCommandHistoryCategory;
xiiActionDescriptorHandle xiiCommandHistoryActions::s_hUndo;
xiiActionDescriptorHandle xiiCommandHistoryActions::s_hRedo;

void xiiCommandHistoryActions::RegisterActions()
{
  s_hCommandHistoryCategory = XII_REGISTER_CATEGORY("CmdHistoryCategory");
  s_hUndo                   = XII_REGISTER_ACTION_AND_DYNAMIC_MENU_1(
    "Document.Undo", xiiActionScope::Document, "Document", "Ctrl+Z", xiiCommandHistoryAction, xiiCommandHistoryAction::ButtonType::Undo);
  s_hRedo = XII_REGISTER_ACTION_AND_DYNAMIC_MENU_1(
    "Document.Redo", xiiActionScope::Document, "Document", "Ctrl+Y", xiiCommandHistoryAction, xiiCommandHistoryAction::ButtonType::Redo);
}

void xiiCommandHistoryActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hCommandHistoryCategory);
  xiiActionManager::UnregisterAction(s_hUndo);
  xiiActionManager::UnregisterAction(s_hRedo);
}

void xiiCommandHistoryActions::MapActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  xiiStringBuilder sSubPath(szPath, "/CmdHistoryCategory");

  pMap->MapAction(s_hCommandHistoryCategory, szPath, 3.0f);
  pMap->MapAction(s_hUndo, sSubPath, 1.0f);
  pMap->MapAction(s_hRedo, sSubPath, 2.0f);
}

xiiCommandHistoryAction::xiiCommandHistoryAction(const xiiActionContext& context, const char* szName, ButtonType button) :
  xiiDynamicActionAndMenuAction(context, szName, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case xiiCommandHistoryAction::ButtonType::Undo:
      SetIconPath(":/GuiFoundation/Icons/Undo16.png");
      break;
    case xiiCommandHistoryAction::ButtonType::Redo:
      SetIconPath(":/GuiFoundation/Icons/Redo16.png");
      break;
  }

  m_Context.m_pDocument->GetCommandHistory()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiCommandHistoryAction::CommandHistoryEventHandler, this));

  UpdateState();
}

xiiCommandHistoryAction::~xiiCommandHistoryAction()
{
  m_Context.m_pDocument->GetCommandHistory()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiCommandHistoryAction::CommandHistoryEventHandler, this));
}

void xiiCommandHistoryAction::GetEntries(xiiHybridArray<xiiDynamicMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();

  xiiCommandHistory* pHistory = m_Context.m_pDocument->GetCommandHistory();

  const xiiUInt32 iCount = (m_ButtonType == ButtonType::Undo) ? pHistory->GetUndoStackSize() : pHistory->GetRedoStackSize();
  for (xiiUInt32 i = 0; i < iCount; i++)
  {
    const xiiCommandTransaction* pTransaction = (m_ButtonType == ButtonType::Undo) ? pHistory->GetUndoStackEntry(i) : pHistory->GetRedoStackEntry(i);
    xiiDynamicMenuAction::Item   entryItem;
    entryItem.m_sDisplay  = pTransaction->m_sDisplayString;
    entryItem.m_UserValue = (xiiUInt32)i + 1; // Number of steps to undo / redo.
    out_Entries.PushBack(entryItem);
  }
}

void xiiCommandHistoryAction::Execute(const xiiVariant& value)
{
  xiiUInt32 iCount = value.IsValid() ? value.ConvertTo<xiiUInt32>() : 1;

  switch (m_ButtonType)
  {
    case ButtonType::Undo:
    {
      XII_ASSERT_DEV(m_Context.m_pDocument->GetCommandHistory()->CanUndo(), "The action should not be active");

      auto stat = m_Context.m_pDocument->GetCommandHistory()->Undo(iCount);
      xiiQtUiServices::MessageBoxStatus(stat, "Could not execute the Undo operation");
    }
    break;

    case ButtonType::Redo:
    {
      XII_ASSERT_DEV(m_Context.m_pDocument->GetCommandHistory()->CanRedo(), "The action should not be active");

      auto stat = m_Context.m_pDocument->GetCommandHistory()->Redo(iCount);
      xiiQtUiServices::MessageBoxStatus(stat, "Could not execute the Redo operation");
    }
    break;
  }
}

void xiiCommandHistoryAction::UpdateState()
{
  switch (m_ButtonType)
  {
    case ButtonType::Undo:
      SetAdditionalDisplayString(m_Context.m_pDocument->GetCommandHistory()->GetUndoDisplayString(), false);
      SetEnabled(m_Context.m_pDocument->GetCommandHistory()->CanUndo());
      break;

    case ButtonType::Redo:
      SetAdditionalDisplayString(m_Context.m_pDocument->GetCommandHistory()->GetRedoDisplayString(), false);
      SetEnabled(m_Context.m_pDocument->GetCommandHistory()->CanRedo());
      break;
  }
}

void xiiCommandHistoryAction::CommandHistoryEventHandler(const xiiCommandHistoryEvent& e)
{
  UpdateState();
}
