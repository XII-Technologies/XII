#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCommand, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiCommand::xiiCommand()  = default;
xiiCommand::~xiiCommand() = default;

bool xiiCommand::HasModifiedDocument() const
{
  if (m_bModifiedDocument)
    return true;

  for (const auto& ca : m_ChildActions)
  {
    if (ca->HasModifiedDocument())
      return true;
  }

  return false;
}

xiiStatus xiiCommand::Do(bool bRedo)
{
  xiiStatus status = DoInternal(bRedo);
  if (status.m_Result == XII_FAILURE)
  {
    if (bRedo)
    {
      // A command that originally succeeded failed on redo!
      return status;
    }
    else
    {
      for (xiiInt32 j = m_ChildActions.GetCount() - 1; j >= 0; --j)
      {
        xiiStatus status2 = m_ChildActions[j]->Undo(true);
        XII_ASSERT_DEV(status2.m_Result == XII_SUCCESS, "Failed do could not be recovered! Inconsistent state!");
      }
      return status;
    }
  }
  if (!bRedo)
    return xiiStatus(XII_SUCCESS);

  const xiiUInt32 uiChildActions = m_ChildActions.GetCount();
  for (xiiUInt32 i = 0; i < uiChildActions; ++i)
  {
    status = m_ChildActions[i]->Do(bRedo);
    if (status.m_Result == XII_FAILURE)
    {
      for (xiiInt32 j = i - 1; j >= 0; --j)
      {
        xiiStatus status2 = m_ChildActions[j]->Undo(true);
        XII_ASSERT_DEV(status2.m_Result == XII_SUCCESS, "Failed redo could not be recovered! Inconsistent state!");
      }
      // A command that originally succeeded failed on redo!
      return status;
    }
  }
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiCommand::Undo(bool bFireEvents)
{
  const xiiUInt32 uiChildActions = m_ChildActions.GetCount();
  for (xiiInt32 i = uiChildActions - 1; i >= 0; --i)
  {
    xiiStatus status = m_ChildActions[i]->Undo(bFireEvents);
    if (status.m_Result == XII_FAILURE)
    {
      for (xiiUInt32 j = i + 1; j < uiChildActions; ++j)
      {
        xiiStatus status2 = m_ChildActions[j]->Do(true);
        XII_ASSERT_DEV(status2.m_Result == XII_SUCCESS, "Failed undo could not be recovered! Inconsistent state!");
      }
      // A command that originally succeeded failed on undo!
      return status;
    }
  }

  xiiStatus status = UndoInternal(bFireEvents);
  if (status.m_Result == XII_FAILURE)
  {
    for (xiiUInt32 j = 0; j < uiChildActions; ++j)
    {
      xiiStatus status2 = m_ChildActions[j]->Do(true);
      XII_ASSERT_DEV(status2.m_Result == XII_SUCCESS, "Failed undo could not be recovered! Inconsistent state!");
    }
    // A command that originally succeeded failed on undo!
    return status;
  }

  return xiiStatus(XII_SUCCESS);
}

void xiiCommand::Cleanup(CommandState state)
{
  CleanupInternal(state);

  for (xiiCommand* pCommand : m_ChildActions)
  {
    pCommand->Cleanup(state);
    pCommand->GetDynamicRTTI()->GetAllocator()->Deallocate(pCommand);
  }

  m_ChildActions.Clear();
}


xiiStatus xiiCommand::AddSubCommand(xiiCommand& command)
{
  xiiCommand*    pCommand = xiiReflectionSerializer::Clone(&command);
  const xiiRTTI* pRtti    = pCommand->GetDynamicRTTI();

  pCommand->m_pDocument = m_pDocument;

  m_ChildActions.PushBack(pCommand);
  m_pDocument->GetCommandHistory()->GetStorage()->m_ActiveCommandStack.PushBack(pCommand);
  xiiStatus ret = pCommand->Do(false);
  m_pDocument->GetCommandHistory()->GetStorage()->m_ActiveCommandStack.PopBack();

  if (ret.m_Result == XII_FAILURE)
  {
    m_ChildActions.PopBack();
    pCommand->GetDynamicRTTI()->GetAllocator()->Deallocate(pCommand);
    return ret;
  }

  if (pCommand->HasReturnValues())
  {
    // Write properties back so any return values get written.
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiMemoryStreamReader         reader(&storage);

    xiiReflectionSerializer::WriteObjectToBinary(writer, pCommand->GetDynamicRTTI(), pCommand);
    xiiReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *pRtti, &command);
  }

  return xiiStatus(XII_SUCCESS);
}
