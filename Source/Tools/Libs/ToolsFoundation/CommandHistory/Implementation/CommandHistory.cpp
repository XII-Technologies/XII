#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCommandTransaction, 1, xiiRTTIDefaultAllocator<xiiCommandTransaction>)
XII_END_DYNAMIC_REFLECTED_TYPE;

////////////////////////////////////////////////////////////////////////
// xiiCommandTransaction
////////////////////////////////////////////////////////////////////////

xiiCommandTransaction::xiiCommandTransaction()
{
  // doesn't do anything on its own
  m_bModifiedDocument = false;
}

xiiCommandTransaction::~xiiCommandTransaction()
{
  XII_ASSERT_DEV(m_ChildActions.IsEmpty(), "The list should be cleared in 'Cleanup'");
}

xiiStatus xiiCommandTransaction::DoInternal(bool bRedo)
{
  XII_ASSERT_DEV(bRedo == true, "Implementation error");
  return XII_SUCCESS;
}

xiiStatus xiiCommandTransaction::UndoInternal(bool bFireEvents)
{
  return XII_SUCCESS;
}

void xiiCommandTransaction::CleanupInternal(CommandState state) {}

xiiStatus xiiCommandTransaction::AddCommandTransaction(xiiCommand* pCommand)
{
  pCommand->m_pDocument = m_pDocument;
  m_ChildActions.PushBack(pCommand);
  return XII_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// xiiCommandHistory
////////////////////////////////////////////////////////////////////////

xiiCommandHistory::xiiCommandHistory(xiiDocument* pDocument)
{
  auto pStorage         = XII_DEFAULT_NEW(Storage);
  pStorage->m_pDocument = pDocument;
  SwapStorage(pStorage);

  m_bTemporaryMode = false;
  m_bIsInUndoRedo  = false;
}

xiiCommandHistory::~xiiCommandHistory()
{
  if (m_pHistoryStorage->GetRefCount() == 1)
  {
    XII_ASSERT_ALWAYS(m_pHistoryStorage->m_UndoHistory.IsEmpty(), "Must clear history before destructor as object manager will be dead already");
    XII_ASSERT_ALWAYS(m_pHistoryStorage->m_RedoHistory.IsEmpty(), "Must clear history before destructor as object manager will be dead already");
  }
}

void xiiCommandHistory::BeginTemporaryCommands(xiiStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands)
{
  XII_ASSERT_DEV(!m_bTemporaryMode, "Temporary Mode cannot be nested");
  StartTransaction(sDisplayString);
  StartTransaction("[Temporary]");

  m_bFireEventsWhenUndoingTempCommands = bFireEventsWhenUndoingTempCommands;
  m_bTemporaryMode                     = true;
  m_iTemporaryDepth                    = (xiiInt32)m_pHistoryStorage->m_TransactionStack.GetCount();
}

void xiiCommandHistory::CancelTemporaryCommands()
{
  EndTemporaryCommands(true);
  EndTransaction(true);
}

void xiiCommandHistory::FinishTemporaryCommands()
{
  EndTemporaryCommands(false);
  EndTransaction(false);
}

bool xiiCommandHistory::InTemporaryTransaction() const
{
  return m_bTemporaryMode;
}

void xiiCommandHistory::SuspendTemporaryTransaction()
{
  m_iPreSuspendTemporaryDepth = (xiiInt32)m_pHistoryStorage->m_TransactionStack.GetCount();
  XII_ASSERT_DEV(m_bTemporaryMode, "No temporary transaction active.");
  while (m_iTemporaryDepth < (xiiInt32)m_pHistoryStorage->m_TransactionStack.GetCount())
  {
    EndTransaction(true);
  }
  EndTemporaryCommands(true);
}

void xiiCommandHistory::ResumeTemporaryTransaction()
{
  XII_ASSERT_DEV(m_iTemporaryDepth == (xiiInt32)m_pHistoryStorage->m_TransactionStack.GetCount() + 1, "Can't resume temporary, not before temporary depth.");
  while (m_iPreSuspendTemporaryDepth > (xiiInt32)m_pHistoryStorage->m_TransactionStack.GetCount())
  {
    StartTransaction("[Temporary]");
  }
  m_bTemporaryMode = true;
  XII_ASSERT_DEV(m_iPreSuspendTemporaryDepth == (xiiInt32)m_pHistoryStorage->m_TransactionStack.GetCount(), "");
}

void xiiCommandHistory::EndTemporaryCommands(bool bCancel)
{
  XII_ASSERT_DEV(m_bTemporaryMode, "Temporary Mode was not enabled");
  XII_ASSERT_DEV(m_iTemporaryDepth == (xiiInt32)m_pHistoryStorage->m_TransactionStack.GetCount(), "Transaction stack is at depth {0} but temporary is at {1}",
                 m_pHistoryStorage->m_TransactionStack.GetCount(), m_iTemporaryDepth);
  m_bTemporaryMode = false;

  EndTransaction(bCancel);
}

xiiStatus xiiCommandHistory::UndoInternal()
{
  XII_ASSERT_DEV(!m_bIsInUndoRedo, "invalidly nested undo/redo");
  XII_ASSERT_DEV(m_pHistoryStorage->m_TransactionStack.IsEmpty(), "Can't undo with active transaction!");
  XII_ASSERT_DEV(!m_pHistoryStorage->m_UndoHistory.IsEmpty(), "Can't undo with empty undo queue!");

  m_bIsInUndoRedo = true;
  {
    xiiCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type      = xiiCommandHistoryEvent::Type::UndoStarted;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }

  xiiCommandTransaction* pTransaction = m_pHistoryStorage->m_UndoHistory.PeekBack();

  xiiStatus status = pTransaction->Undo(true);
  if (status.Succeeded())
  {
    m_pHistoryStorage->m_UndoHistory.PopBack();
    m_pHistoryStorage->m_RedoHistory.PushBack(pTransaction);

    m_pHistoryStorage->m_pDocument->SetModified(true);

    status = xiiStatus(XII_SUCCESS);
  }

  m_bIsInUndoRedo = false;
  {
    xiiCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type      = xiiCommandHistoryEvent::Type::UndoEnded;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }
  return status;
}

xiiStatus xiiCommandHistory::Undo(xiiUInt32 uiNumEntries)
{
  for (xiiUInt32 i = 0; i < uiNumEntries; i++)
  {
    XII_SUCCEED_OR_RETURN(UndoInternal());
  }

  return XII_SUCCESS;
}

xiiStatus xiiCommandHistory::RedoInternal()
{
  XII_ASSERT_DEV(!m_bIsInUndoRedo, "invalidly nested undo/redo");
  XII_ASSERT_DEV(m_pHistoryStorage->m_TransactionStack.IsEmpty(), "Can't redo with active transaction!");
  XII_ASSERT_DEV(!m_pHistoryStorage->m_RedoHistory.IsEmpty(), "Can't redo with empty undo queue!");

  m_bIsInUndoRedo = true;
  {
    xiiCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type      = xiiCommandHistoryEvent::Type::RedoStarted;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }

  xiiCommandTransaction* pTransaction = m_pHistoryStorage->m_RedoHistory.PeekBack();

  xiiStatus status(XII_FAILURE);
  if (pTransaction->Do(true).Succeeded())
  {
    m_pHistoryStorage->m_RedoHistory.PopBack();
    m_pHistoryStorage->m_UndoHistory.PushBack(pTransaction);

    m_pHistoryStorage->m_pDocument->SetModified(true);

    status = xiiStatus(XII_SUCCESS);
  }

  m_bIsInUndoRedo = false;
  {
    xiiCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type      = xiiCommandHistoryEvent::Type::RedoEnded;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }
  return status;
}

xiiStatus xiiCommandHistory::Redo(xiiUInt32 uiNumEntries)
{
  for (xiiUInt32 i = 0; i < uiNumEntries; i++)
  {
    XII_SUCCEED_OR_RETURN(RedoInternal());
  }

  return XII_SUCCESS;
}

bool xiiCommandHistory::CanUndo() const
{
  if (!m_pHistoryStorage->m_TransactionStack.IsEmpty())
    return false;

  return !m_pHistoryStorage->m_UndoHistory.IsEmpty();
}

bool xiiCommandHistory::CanRedo() const
{
  if (!m_pHistoryStorage->m_TransactionStack.IsEmpty())
    return false;

  return !m_pHistoryStorage->m_RedoHistory.IsEmpty();
}

xiiStringView xiiCommandHistory::GetUndoDisplayString() const
{
  if (m_pHistoryStorage->m_UndoHistory.IsEmpty())
    return "";

  return m_pHistoryStorage->m_UndoHistory.PeekBack()->m_sDisplayString;
}

xiiStringView xiiCommandHistory::GetRedoDisplayString() const
{
  if (m_pHistoryStorage->m_RedoHistory.IsEmpty())
    return "";

  return m_pHistoryStorage->m_RedoHistory.PeekBack()->m_sDisplayString;
}

void xiiCommandHistory::StartTransaction(const xiiFormatString& displayString)
{
  XII_ASSERT_DEV(!m_bIsInUndoRedo, "Cannot start new transaction while redoing/undoing.");

  /// \todo Allow to have a limited transaction history and clean up transactions after a while

  xiiCommandTransaction* pTransaction;

  if (m_bTemporaryMode && !m_pHistoryStorage->m_TransactionStack.IsEmpty())
  {
    pTransaction = m_pHistoryStorage->m_TransactionStack.PeekBack();
    pTransaction->Undo(m_bFireEventsWhenUndoingTempCommands).IgnoreResult();
    pTransaction->Cleanup(xiiCommand::CommandState::WasUndone);
    m_pHistoryStorage->m_TransactionStack.PushBack(pTransaction);
    m_pHistoryStorage->m_ActiveCommandStack.PushBack(pTransaction);
    return;
  }

  xiiStringBuilder tmp;

  pTransaction                   = xiiGetStaticRTTI<xiiCommandTransaction>()->GetAllocator()->Allocate<xiiCommandTransaction>();
  pTransaction->m_pDocument      = m_pHistoryStorage->m_pDocument;
  pTransaction->m_sDisplayString = displayString.GetText(tmp);

  if (!m_pHistoryStorage->m_TransactionStack.IsEmpty())
  {
    // Stacked transaction
    m_pHistoryStorage->m_TransactionStack.PeekBack()->AddCommandTransaction(pTransaction).AssertSuccess();
    m_pHistoryStorage->m_TransactionStack.PushBack(pTransaction);
    m_pHistoryStorage->m_ActiveCommandStack.PushBack(pTransaction);
  }
  else
  {
    // Initial transaction
    m_pHistoryStorage->m_TransactionStack.PushBack(pTransaction);
    m_pHistoryStorage->m_ActiveCommandStack.PushBack(pTransaction);
    {
      xiiCommandHistoryEvent e;
      e.m_pDocument = m_pHistoryStorage->m_pDocument;
      e.m_Type      = xiiCommandHistoryEvent::Type::TransactionStarted;
      m_pHistoryStorage->m_Events.Broadcast(e);
    }
  }
  return;
}

void xiiCommandHistory::EndTransaction(bool bCancel)
{
  XII_ASSERT_DEV(!m_pHistoryStorage->m_TransactionStack.IsEmpty(), "Trying to end transaction without starting one!");

  if (m_pHistoryStorage->m_TransactionStack.GetCount() == 1)
  {
    /// Empty transactions are always canceled, so that they do not create an unnecessary undo action and clear the redo stack

    const bool bDidAnything = m_pHistoryStorage->m_TransactionStack.PeekBack()->HasChildActions();
    if (!bDidAnything)
      bCancel = true;

    xiiCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type      = bCancel ? xiiCommandHistoryEvent::Type::BeforeTransactionCanceled : xiiCommandHistoryEvent::Type::BeforeTransactionEnded;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }

  if (!bCancel)
  {
    if (m_pHistoryStorage->m_TransactionStack.GetCount() > 1)
    {
      m_pHistoryStorage->m_TransactionStack.PopBack();
      m_pHistoryStorage->m_ActiveCommandStack.PopBack();
    }
    else
    {
      const bool bDidModifyDoc = m_pHistoryStorage->m_TransactionStack.PeekBack()->HasModifiedDocument();
      m_pHistoryStorage->m_UndoHistory.PushBack(m_pHistoryStorage->m_TransactionStack.PeekBack());
      m_pHistoryStorage->m_TransactionStack.PopBack();
      m_pHistoryStorage->m_ActiveCommandStack.PopBack();
      ClearRedoHistory();

      if (bDidModifyDoc)
      {
        m_pHistoryStorage->m_pDocument->SetModified(true);
      }
    }
  }
  else
  {
    xiiCommandTransaction* pTransaction = m_pHistoryStorage->m_TransactionStack.PeekBack();

    pTransaction->Undo(true).AssertSuccess();
    m_pHistoryStorage->m_TransactionStack.PopBack();
    m_pHistoryStorage->m_ActiveCommandStack.PopBack();

    if (m_pHistoryStorage->m_TransactionStack.IsEmpty())
    {
      pTransaction->Cleanup(xiiCommand::CommandState::WasUndone);
      pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);
    }
  }

  if (m_pHistoryStorage->m_TransactionStack.IsEmpty())
  {
    // All transactions done
    xiiCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type      = bCancel ? xiiCommandHistoryEvent::Type::TransactionCanceled : xiiCommandHistoryEvent::Type::TransactionEnded;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }
}

xiiStatus xiiCommandHistory::AddCommand(xiiCommand& ref_command)
{
  XII_ASSERT_DEV(!m_pHistoryStorage->m_TransactionStack.IsEmpty(), "Cannot add command while no transaction is started");
  XII_ASSERT_DEV(!m_pHistoryStorage->m_ActiveCommandStack.IsEmpty(), "Transaction stack is not synced anymore with m_ActiveCommandStack");

  auto res = m_pHistoryStorage->m_ActiveCommandStack.PeekBack()->AddSubCommand(ref_command);

  // Error handling should be on the caller side.
  // if (res.Failed() && !res.m_sMessage.IsEmpty())
  // {
  //   xiiLog::Error("Command failed: '{0}'", res.m_sMessage);
  // }

  return res;
}

void xiiCommandHistory::ClearUndoHistory()
{
  XII_ASSERT_DEV(!m_bIsInUndoRedo, "Cannot clear undo/redo history while redoing/undoing.");
  while (!m_pHistoryStorage->m_UndoHistory.IsEmpty())
  {
    xiiCommandTransaction* pTransaction = m_pHistoryStorage->m_UndoHistory.PeekBack();

    pTransaction->Cleanup(xiiCommand::CommandState::WasDone);
    pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);

    m_pHistoryStorage->m_UndoHistory.PopBack();
  }
}

void xiiCommandHistory::ClearRedoHistory()
{
  XII_ASSERT_DEV(!m_bIsInUndoRedo, "Cannot clear undo/redo history while redoing/undoing.");
  while (!m_pHistoryStorage->m_RedoHistory.IsEmpty())
  {
    xiiCommandTransaction* pTransaction = m_pHistoryStorage->m_RedoHistory.PeekBack();

    pTransaction->Cleanup(xiiCommand::CommandState::WasUndone);
    pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);

    m_pHistoryStorage->m_RedoHistory.PopBack();
  }
}

void xiiCommandHistory::MergeLastTwoTransactions()
{
  /// \todo This would not be necessary, if hierarchical transactions would not crash

  XII_ASSERT_DEV(m_pHistoryStorage->m_RedoHistory.IsEmpty(), "This can only be called directly after EndTransaction, when the redo history is empty");
  XII_ASSERT_DEV(m_pHistoryStorage->m_UndoHistory.GetCount() >= 2, "Can only do this when at least two transcations are in the queue");

  xiiCommandTransaction* pLast = m_pHistoryStorage->m_UndoHistory.PeekBack();
  m_pHistoryStorage->m_UndoHistory.PopBack();

  xiiCommandTransaction* pNowLast = m_pHistoryStorage->m_UndoHistory.PeekBack();
  pNowLast->m_ChildActions.PushBackRange(pLast->m_ChildActions);

  pLast->m_ChildActions.Clear();

  pLast->GetDynamicRTTI()->GetAllocator()->Deallocate(pLast);
}

xiiUInt32 xiiCommandHistory::GetUndoStackSize() const
{
  return m_pHistoryStorage->m_UndoHistory.GetCount();
}

xiiUInt32 xiiCommandHistory::GetRedoStackSize() const
{
  return m_pHistoryStorage->m_RedoHistory.GetCount();
}

const xiiCommandTransaction* xiiCommandHistory::GetUndoStackEntry(xiiUInt32 uiIndex) const
{
  return m_pHistoryStorage->m_UndoHistory[GetUndoStackSize() - 1 - uiIndex];
}

const xiiCommandTransaction* xiiCommandHistory::GetRedoStackEntry(xiiUInt32 uiIndex) const
{
  return m_pHistoryStorage->m_RedoHistory[GetRedoStackSize() - 1 - uiIndex];
}

xiiSharedPtr<xiiCommandHistory::Storage> xiiCommandHistory::SwapStorage(xiiSharedPtr<xiiCommandHistory::Storage> pNewStorage)
{
  XII_ASSERT_ALWAYS(pNewStorage != nullptr, "Need a valid history storage object");

  XII_ASSERT_DEV(!m_bIsInUndoRedo, "Can't be in Undo/Redo when swapping storage.");

  auto retVal = m_pHistoryStorage;

  m_EventsUnsubscriber.Unsubscribe();

  m_pHistoryStorage = pNewStorage;

  m_pHistoryStorage->m_Events.AddEventHandler([this](const xiiCommandHistoryEvent& e) { m_Events.Broadcast(e); }, m_EventsUnsubscriber);

  return retVal;
}
