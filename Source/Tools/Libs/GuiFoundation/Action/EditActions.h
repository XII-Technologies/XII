#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
///
class XII_GUIFOUNDATION_DLL xiiEditActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath, bool bDeleteAction, bool bAdvancedPasteActions);
  static void MapContextMenuActions(const char* szMapping, const char* szPath);
  static void MapViewContextMenuActions(const char* szMapping, const char* szPath);

  static xiiActionDescriptorHandle s_hEditCategory;
  static xiiActionDescriptorHandle s_hCopy;
  static xiiActionDescriptorHandle s_hPaste;
  static xiiActionDescriptorHandle s_hPasteAsChild;
  static xiiActionDescriptorHandle s_hPasteAtOriginalLocation;
  static xiiActionDescriptorHandle s_hDelete;
};


///
class XII_GUIFOUNDATION_DLL xiiEditAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditAction, xiiButtonAction);

public:
  enum class ButtonType
  {
    Copy,
    Paste,
    PasteAsChild,
    PasteAtOriginalLocation,
    Delete,
  };
  xiiEditAction(const xiiActionContext& context, const char* szName, ButtonType button);
  ~xiiEditAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  void SelectionEventHandler(const xiiSelectionManagerEvent& e);

  ButtonType m_ButtonType;
};
