/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiCommandHistory;

class XII_TOOLSFOUNDATION_DLL xiiCommandTransaction : public xiiCommand
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCommandTransaction, xiiCommand);

public:
  xiiCommandTransaction();
  ~xiiCommandTransaction();

  xiiString m_sDisplayString;

private:
  virtual xiiStatus DoInternal(bool bRedo) override;
  virtual xiiStatus UndoInternal(bool bFireEvents) override;
  virtual void      CleanupInternal(CommandState state) override;
  xiiStatus         AddCommandTransaction(xiiCommand* command);

private:
  friend class xiiCommandHistory;
};

struct xiiCommandHistoryEvent
{
  enum class Type
  {
    UndoStarted,
    UndoEnded,
    RedoStarted,
    RedoEnded,
    TransactionStarted,        ///< Emit after initial transaction started.
    BeforeTransactionEnded,    ///< Emit before initial transaction ended.
    BeforeTransactionCanceled, ///< Emit before initial transaction ended.
    TransactionEnded,          ///< Emit after initial transaction ended.
    TransactionCanceled,       ///< Emit after initial transaction canceled.
    HistoryChanged,
  };

  Type               m_Type;
  const xiiDocument* m_pDocument;
};

/// Stores the undo / redo stacks of transactions done on a document.
class XII_TOOLSFOUNDATION_DLL xiiCommandHistory
{
public:
  xiiEvent<const xiiCommandHistoryEvent&, xiiMutex> m_Events;

  // Storage for the command history so it can be swapped when using multiple sub documents.
  class Storage : public xiiRefCounted
  {
  public:
    xiiHybridArray<xiiCommandTransaction*, 4>         m_TransactionStack;
    xiiHybridArray<xiiCommand*, 4>                    m_ActiveCommandStack;
    xiiDeque<xiiCommandTransaction*>                  m_UndoHistory;
    xiiDeque<xiiCommandTransaction*>                  m_RedoHistory;
    xiiDocument*                                      m_pDocument = nullptr;
    xiiEvent<const xiiCommandHistoryEvent&, xiiMutex> m_Events;
  };

public:
  xiiCommandHistory(xiiDocument* pDocument);
  ~xiiCommandHistory();

  const xiiDocument* GetDocument() const { return m_pHistoryStorage->m_pDocument; }

  xiiStatus Undo(xiiUInt32 uiNumEntries = 1);
  xiiStatus Redo(xiiUInt32 uiNumEntries = 1);

  bool CanUndo() const;
  bool CanRedo() const;

  xiiStringView GetUndoDisplayString() const;
  xiiStringView GetRedoDisplayString() const;

  void StartTransaction(const xiiFormatString& displayString);
  void CancelTransaction() { EndTransaction(true); }
  void FinishTransaction() { EndTransaction(false); }

  /// Returns true, if between StartTransaction / EndTransaction. False during Undo/Redo.
  bool IsInTransaction() const { return !m_pHistoryStorage->m_TransactionStack.IsEmpty(); }
  bool IsInUndoRedo() const { return m_bIsInUndoRedo; }

  /// Call this to start a series of transactions that typically change the same value over and over (e.g. dragging an object to a position).
  /// Every time a new transaction is started, the previous one is undone first. At the end of a series of temporary transactions, only the last
  /// transaction will be stored as a single undo step. Call this first and then start a transaction inside it.
  void BeginTemporaryCommands(xiiStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands = false);
  void CancelTemporaryCommands();
  void FinishTemporaryCommands();

  bool InTemporaryTransaction() const;
  void SuspendTemporaryTransaction();
  void ResumeTemporaryTransaction();

  xiiStatus AddCommand(xiiCommand& ref_command);

  void ClearUndoHistory();
  void ClearRedoHistory();

  void MergeLastTwoTransactions();

  xiiUInt32                    GetUndoStackSize() const;
  xiiUInt32                    GetRedoStackSize() const;
  const xiiCommandTransaction* GetUndoStackEntry(xiiUInt32 uiIndex) const;
  const xiiCommandTransaction* GetRedoStackEntry(xiiUInt32 uiIndex) const;

  xiiSharedPtr<xiiCommandHistory::Storage> SwapStorage(xiiSharedPtr<xiiCommandHistory::Storage> pNewStorage);
  xiiSharedPtr<xiiCommandHistory::Storage> GetStorage() { return m_pHistoryStorage; }

private:
  friend class xiiCommand;

  xiiStatus UndoInternal();
  xiiStatus RedoInternal();

  void EndTransaction(bool bCancel);
  void EndTemporaryCommands(bool bCancel);

  xiiSharedPtr<xiiCommandHistory::Storage> m_pHistoryStorage;

  xiiEvent<const xiiCommandHistoryEvent&, xiiMutex>::Unsubscriber m_EventsUnsubscriber;

  bool     m_bFireEventsWhenUndoingTempCommands = false;
  bool     m_bTemporaryMode                     = false;
  xiiInt32 m_iTemporaryDepth                    = -1;
  xiiInt32 m_iPreSuspendTemporaryDepth          = -1;
  bool     m_bIsInUndoRedo                      = false;
};
