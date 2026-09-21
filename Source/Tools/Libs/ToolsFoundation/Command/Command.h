/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiDocument;
class xiiCommandTransaction;

/// Interface for a command
///
/// Commands are the only objects that have non-const access to any data structures (contexts, documents etc.).
/// Thus, any modification must go through a command and the xiiCommandHistory is the only class capable of executing commands.
class XII_TOOLSFOUNDATION_DLL xiiCommand : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCommand, xiiReflectedClass);

public:
  xiiCommand();
  ~xiiCommand();

  bool IsUndoable() const { return m_bUndoable; };
  bool HasChildActions() const { return !m_ChildActions.IsEmpty(); }
  bool HasModifiedDocument() const;

  enum class CommandState
  {
    WasDone,
    WasUndone
  };

protected:
  xiiStatus Do(bool bRedo);
  xiiStatus Undo(bool bFireEvents);
  void      Cleanup(CommandState state);

  xiiStatus    AddSubCommand(xiiCommand& command);
  xiiDocument* GetDocument() { return m_pDocument; };

private:
  virtual bool      HasReturnValues() const { return false; }
  virtual xiiStatus DoInternal(bool bRedo)              = 0;
  virtual xiiStatus UndoInternal(bool bFireEvents)      = 0;
  virtual void      CleanupInternal(CommandState state) = 0;

protected:
  friend class xiiCommandHistory;
  friend class xiiCommandTransaction;

  xiiString                      m_sDescription;
  bool                           m_bUndoable         = true;
  bool                           m_bModifiedDocument = true;
  xiiHybridArray<xiiCommand*, 8> m_ChildActions;
  xiiDocument*                   m_pDocument = nullptr;
};
