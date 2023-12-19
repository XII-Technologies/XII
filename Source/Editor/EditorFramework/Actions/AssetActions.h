#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class XII_EDITORFRAMEWORK_DLL xiiAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(xiiStringView sMapping);
  static void MapToolBarActions(xiiStringView sMapping, bool bDocument);

  static xiiActionDescriptorHandle s_hAssetCategory;
  static xiiActionDescriptorHandle s_hTransformAsset;
  static xiiActionDescriptorHandle s_hTransformAllAssets;
  static xiiActionDescriptorHandle s_hResaveAllAssets;
  static xiiActionDescriptorHandle s_hCheckFileSystem;
  static xiiActionDescriptorHandle s_hWriteLookupTable;
  static xiiActionDescriptorHandle s_hWriteDependencyDGML;
};

///
class XII_EDITORFRAMEWORK_DLL xiiAssetAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAssetAction, xiiButtonAction);

public:
  enum class ButtonType
  {
    TransformAsset,
    TransformAllAssets,
    ResaveAllAssets,
    CheckFileSystem,
    WriteLookupTable,
    WriteDependencyDGML,
  };

  xiiAssetAction(const xiiActionContext& context, const char* szName, ButtonType button);
  ~xiiAssetAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  ButtonType m_ButtonType;
};
