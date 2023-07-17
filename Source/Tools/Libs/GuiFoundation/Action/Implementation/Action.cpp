#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/ActionManager.h>

const xiiActionDescriptor* xiiActionDescriptorHandle::GetDescriptor() const
{
  return xiiActionManager::GetActionDescriptor(*this);
}

xiiActionDescriptor::xiiActionDescriptor(xiiActionType::Enum type, xiiActionScope::Enum scope, xiiStringView sName, xiiStringView sCategoryPath, xiiStringView sShortcut, CreateActionFunc createAction, DeleteActionFunc deleteAction) :
  m_Type(type), m_Scope(scope), m_sActionName(sName), m_sCategoryPath(sCategoryPath), m_sShortcut(sShortcut), m_sDefaultShortcut(sShortcut), m_CreateAction(createAction), m_DeleteAction(deleteAction)
{
}

xiiAction* xiiActionDescriptor::CreateAction(const xiiActionContext& context) const
{
  XII_ASSERT_DEV(!m_Handle.IsInvalidated(), "Handle invalid!");
  auto pAction                 = m_CreateAction(context);
  pAction->m_hDescriptorHandle = m_Handle;

  m_CreatedActions.PushBack(pAction);
  return pAction;
}

void xiiActionDescriptor::DeleteAction(xiiAction* pAction) const
{
  m_CreatedActions.RemoveAndSwap(pAction);

  if (m_DeleteAction == nullptr)
  {
    XII_DEFAULT_DELETE(pAction);
  }
  else
    m_DeleteAction(pAction);
}


void xiiActionDescriptor::UpdateExistingActions()
{
  for (auto pAction : m_CreatedActions)
  {
    pAction->TriggerUpdate();
  }
}

void xiiAction::TriggerUpdate()
{
  m_StatusUpdateEvent.Broadcast(this);
}

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
