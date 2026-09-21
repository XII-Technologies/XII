/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QKeySequence>
#include <ToolsFoundation/Document/DocumentManager.h>

class QWidget;
struct xiiActionDescriptor;
class xiiAction;
struct xiiActionContext;

using xiiActionId      = xiiGenericId<24, 8>;
using CreateActionFunc = xiiAction* (*)(const xiiActionContext&);
using DeleteActionFunc = void (*)(xiiAction*);

/// Handle for a xiiAction.
///
/// xiiAction can be invalidated at runtime so don't store them.
class XII_GUIFOUNDATION_DLL xiiActionDescriptorHandle
{
public:
  using StorageType = xiiUInt32;

  XII_DECLARE_HANDLE_TYPE(xiiActionDescriptorHandle, xiiActionId);
  friend class xiiActionManager;

public:
  const xiiActionDescriptor* GetDescriptor() const;
};

///
struct xiiActionScope
{
  enum Enum
  {
    Global,
    Document,
    Window,
    Default = Global
  };
  using StorageType = xiiUInt8;
};

///
struct xiiActionType
{
  enum Enum
  {
    Action,
    Category,
    Menu,
    ActionAndMenu,
    Default = Action
  };
  using StorageType = xiiUInt8;
};

///
struct XII_GUIFOUNDATION_DLL xiiActionContext
{
  xiiActionContext() = default;
  xiiActionContext(xiiDocument* pDoc) { m_pDocument = pDoc; }

  xiiDocument* m_pDocument = nullptr;
  xiiString    m_sMapping;
  QWidget*     m_pWindow = nullptr;
};


///
struct XII_GUIFOUNDATION_DLL xiiActionDescriptor
{
  xiiActionDescriptor() = default;
  xiiActionDescriptor(xiiActionType::Enum type, xiiActionScope::Enum scope, xiiStringView sName, xiiStringView sCategoryPath, xiiStringView sShortcut, CreateActionFunc createAction, DeleteActionFunc deleteAction = nullptr);

  xiiActionDescriptorHandle m_Handle;
  xiiEnum<xiiActionType>    m_Type;

  xiiEnum<xiiActionScope> m_Scope;
  xiiString               m_sActionName;   ///< Unique within category path, shown in key configuration dialog
  xiiString               m_sCategoryPath; ///< Category in key configuration dialog, e.g. "Tree View" or "File"

  xiiString m_sShortcut;
  xiiString m_sDefaultShortcut;

  xiiAction* CreateAction(const xiiActionContext& context) const;
  void       DeleteAction(xiiAction* pAction) const;

  void UpdateExistingActions();

private:
  CreateActionFunc m_CreateAction;
  DeleteActionFunc m_DeleteAction;

  mutable xiiHybridArray<xiiAction*, 4> m_CreatedActions;
};


///
class XII_GUIFOUNDATION_DLL xiiAction : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAction, xiiReflectedClass);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiAction);

public:
  xiiAction(const xiiActionContext& context) { m_Context = context; }
  virtual void Execute(const xiiVariant& value) = 0;

  void                      TriggerUpdate();
  const xiiActionContext&   GetContext() const { return m_Context; }
  xiiActionDescriptorHandle GetDescriptorHandle() { return m_hDescriptorHandle; }

public:
  xiiEvent<xiiAction*> m_StatusUpdateEvent; ///< Fire when the state of the action changes (enabled, value etc...)

protected:
  xiiActionContext m_Context;

private:
  friend struct xiiActionDescriptor;
  xiiActionDescriptorHandle m_hDescriptorHandle;
};
