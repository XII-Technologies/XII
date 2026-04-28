/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>

///
class XII_GUIFOUNDATION_DLL xiiCommandHistoryActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(xiiStringView sMapping, xiiStringView sTargetMenu = "G.Edit");

  static xiiActionDescriptorHandle s_hCommandHistoryCategory;
  static xiiActionDescriptorHandle s_hUndo;
  static xiiActionDescriptorHandle s_hRedo;
};


///
class XII_GUIFOUNDATION_DLL xiiCommandHistoryAction : public xiiDynamicActionAndMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCommandHistoryAction, xiiDynamicActionAndMenuAction);

public:
  enum class ButtonType
  {
    Undo,
    Redo,
  };

  xiiCommandHistoryAction(const xiiActionContext& context, xiiStringView sName, ButtonType button);
  ~xiiCommandHistoryAction();

  virtual void Execute(const xiiVariant& value) override;
  virtual void GetEntries(xiiDynamicArray<Item>& out_entries) override;

private:
  void UpdateState();
  void CommandHistoryEventHandler(const xiiCommandHistoryEvent& e);

  ButtonType m_ButtonType;
};
