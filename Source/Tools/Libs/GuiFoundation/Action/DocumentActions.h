#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

///
class XII_GUIFOUNDATION_DLL xiiDocumentActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(xiiStringView sMapping, xiiStringView sPath, bool bForToolbar);
  static void MapToolsActions(xiiStringView sMapping, xiiStringView sPath);

  static xiiActionDescriptorHandle s_hSaveCategory;
  static xiiActionDescriptorHandle s_hSave;
  static xiiActionDescriptorHandle s_hSaveAs;
  static xiiActionDescriptorHandle s_hSaveAll;

  static xiiActionDescriptorHandle s_hCloseCategory;
  static xiiActionDescriptorHandle s_hClose;
  static xiiActionDescriptorHandle s_hCloseAll;
  static xiiActionDescriptorHandle s_hCloseAllButThis;

  static xiiActionDescriptorHandle s_hOpenContainingFolder;
  static xiiActionDescriptorHandle s_hCopyAssetGuid;

  static xiiActionDescriptorHandle s_hUpdatePrefabs;
  static xiiActionDescriptorHandle s_hDocumentCategory;
};


/// \brief Standard document actions.
class XII_GUIFOUNDATION_DLL xiiDocumentAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDocumentAction, xiiButtonAction);

public:
  enum class ButtonType
  {
    Save,
    SaveAs,
    SaveAll,
    Close,
    CloseAll,
    CloseAllButThis,
    OpenContainingFolder,
    UpdatePrefabs,
    CopyAssetGuid,
  };
  xiiDocumentAction(const xiiActionContext& context, xiiStringView sName, ButtonType button);
  ~xiiDocumentAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  void DocumentEventHandler(const xiiDocumentEvent& e);

  ButtonType m_ButtonType;
};
