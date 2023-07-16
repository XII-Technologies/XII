#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

struct xiiStandardMenuTypes
{
  using StorageType = xiiUInt32;

  enum Enum
  {
    File    = XII_BIT(0),
    Edit    = XII_BIT(1),
    Panels  = XII_BIT(2),
    Project = XII_BIT(3),
    Scene   = XII_BIT(4),
    View    = XII_BIT(5),
    Help    = XII_BIT(6),

    Default = 0
  };

  struct Bits
  {
    StorageType File : 1;
    StorageType Edit : 1;
    StorageType Panels : 1;
    StorageType Project : 1;
    StorageType Scene : 1;
    StorageType View : 1;
    StorageType Help : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiStandardMenuTypes);

///
class XII_GUIFOUNDATION_DLL xiiStandardMenus
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const xiiBitflags<xiiStandardMenuTypes>& menus);

  static xiiActionDescriptorHandle s_hMenuFile;
  static xiiActionDescriptorHandle s_hMenuEdit;
  static xiiActionDescriptorHandle s_hMenuPanels;
  static xiiActionDescriptorHandle s_hMenuProject;
  static xiiActionDescriptorHandle s_hMenuScene;
  static xiiActionDescriptorHandle s_hMenuView;
  static xiiActionDescriptorHandle s_hMenuHelp;
  static xiiActionDescriptorHandle s_hCheckForUpdates;
  static xiiActionDescriptorHandle s_hReportProblem;
};

///
class XII_GUIFOUNDATION_DLL xiiApplicationPanelsMenuAction : public xiiDynamicMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiApplicationPanelsMenuAction, xiiDynamicMenuAction);

public:
  xiiApplicationPanelsMenuAction(const xiiActionContext& context, const char* szName, const char* szIconPath) :
    xiiDynamicMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(xiiHybridArray<xiiDynamicMenuAction::Item, 16>& out_entries) override;
  virtual void Execute(const xiiVariant& value) override;
};

//////////////////////////////////////////////////////////////////////////

class XII_GUIFOUNDATION_DLL xiiHelpActions : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiHelpActions, xiiButtonAction);

public:
  enum class ButtonType
  {
    CheckForUpdates,
    ReportProblem,
  };

  xiiHelpActions(const xiiActionContext& context, const char* szName, ButtonType button);
  ~xiiHelpActions();

  virtual void Execute(const xiiVariant& value) override;

private:
  ButtonType m_ButtonType;
};
