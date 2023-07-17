#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>

#define XII_REGISTER_ACTION_0(ActionName, Scope, CategoryName, ShortCut, ActionClass)                                    \
  xiiActionManager::RegisterAction(xiiActionDescriptor(xiiActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
                                                       [](const xiiActionContext& context) -> xiiAction* { return XII_DEFAULT_NEW(ActionClass, context, ActionName); }));

#define XII_REGISTER_ACTION_1(ActionName, Scope, CategoryName, ShortCut, ActionClass, Param1)                            \
  xiiActionManager::RegisterAction(xiiActionDescriptor(xiiActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
                                                       [](const xiiActionContext& context) -> xiiAction* { return XII_DEFAULT_NEW(ActionClass, context, ActionName, Param1); }));

#define XII_REGISTER_ACTION_2(ActionName, Scope, CategoryName, ShortCut, ActionClass, Param1, Param2)                    \
  xiiActionManager::RegisterAction(xiiActionDescriptor(xiiActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
                                                       [](const xiiActionContext& context) -> xiiAction* { return XII_DEFAULT_NEW(ActionClass, context, ActionName, Param1, Param2); }));

#define XII_REGISTER_DYNAMIC_MENU(ActionName, ActionClass, IconPath)                                                     \
  xiiActionManager::RegisterAction(xiiActionDescriptor(xiiActionType::Menu, xiiActionScope::Default, ActionName, "", "", \
                                                       [](const xiiActionContext& context) -> xiiAction* { return XII_DEFAULT_NEW(ActionClass, context, ActionName, IconPath); }));

#define XII_REGISTER_ACTION_AND_DYNAMIC_MENU_1(ActionName, Scope, CategoryName, ShortCut, ActionClass, Param1)                  \
  xiiActionManager::RegisterAction(xiiActionDescriptor(xiiActionType::ActionAndMenu, Scope, ActionName, CategoryName, ShortCut, \
                                                       [](const xiiActionContext& context) -> xiiAction* { return XII_DEFAULT_NEW(ActionClass, context, ActionName, Param1); }));

#define XII_REGISTER_MENU(ActionName)                                                                                    \
  xiiActionManager::RegisterAction(xiiActionDescriptor(xiiActionType::Menu, xiiActionScope::Default, ActionName, "", "", \
                                                       [](const xiiActionContext& context) -> xiiAction* { return XII_DEFAULT_NEW(xiiMenuAction, context, ActionName, ""); }));

#define XII_REGISTER_MENU_WITH_ICON(ActionName, IconPath)                                                                \
  xiiActionManager::RegisterAction(xiiActionDescriptor(xiiActionType::Menu, xiiActionScope::Default, ActionName, "", "", \
                                                       [](const xiiActionContext& context) -> xiiAction* { return XII_DEFAULT_NEW(xiiMenuAction, context, ActionName, IconPath); }));

#define XII_REGISTER_CATEGORY(CategoryName)                                                                                    \
  xiiActionManager::RegisterAction(xiiActionDescriptor(xiiActionType::Category, xiiActionScope::Default, CategoryName, "", "", \
                                                       [](const xiiActionContext& context) -> xiiAction* { return XII_DEFAULT_NEW(xiiCategoryAction, context); }));

///
class XII_GUIFOUNDATION_DLL xiiActionManager
{
public:
  static xiiActionDescriptorHandle  RegisterAction(const xiiActionDescriptor& desc);
  static bool                       UnregisterAction(xiiActionDescriptorHandle& ref_hAction);
  static const xiiActionDescriptor* GetActionDescriptor(xiiActionDescriptorHandle hAction);
  static xiiActionDescriptorHandle  GetActionHandle(xiiStringView sCategory, xiiStringView sActionName);

  /// \brief Searches all action categories for the given action name. Returns the category name in which the action name was found, or an empty
  /// string.
  static xiiString FindActionCategory(xiiStringView sActionName);

  /// \brief Quick way to execute an action from code
  ///
  /// The use case is mostly for unit tests, which need to execute actions directly and without a link dependency on
  /// the code that registered the action.
  ///
  /// \param szCategory The category of the action, ie. under which name the action appears in the Shortcut binding dialog.
  ///        For example "Scene", "Scene - Cameras", "Scene - Selection", "Assets" etc.
  ///        This parameter may be nullptr in which case FindActionCategory(szActionName) is used to try to detect the category automatically.
  /// \param szActionName The name (not mapped path) under which the action was registered.
  ///        For example "Selection.Copy", "Prefabs.ConvertToEngine", "Scene.Camera.SnapObjectToCamera"
  /// \param context The context in which to execute the action. Depending on the xiiActionScope of the target action,
  ///        some members are optional. E.g. for document actions, only the m_pDocument member must be specified.
  /// \param value Optional value passed through to the xiiAction::Execute() call. Some actions use it, most don't.
  /// \return Returns failure in case the action could not be found.
  static xiiResult ExecuteAction(xiiStringView sCategory, xiiStringView sActionName, const xiiActionContext& context, const xiiVariant& value = xiiVariant());

  static void SaveShortcutAssignment();
  static void LoadShortcutAssignment();

  static const xiiIdTable<xiiActionId, xiiActionDescriptor*>::ConstIterator GetActionIterator();

  struct Event
  {
    enum class Type
    {
      ActionAdded,
      ActionRemoved
    };

    Type                       m_Type;
    const xiiActionDescriptor* m_pDesc;
    xiiActionDescriptorHandle  m_Handle;
  };

  static xiiEvent<const Event&> s_Events;

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, ActionManager);

  static void                 Startup();
  static void                 Shutdown();
  static xiiActionDescriptor* CreateActionDesc(const xiiActionDescriptor& desc);
  static void                 DeleteActionDesc(xiiActionDescriptor* pDesc);

  struct CategoryData
  {
    xiiSet<xiiActionDescriptorHandle>                    m_Actions;
    xiiHashTable<xiiStringView, xiiActionDescriptorHandle> m_ActionNameToHandle;
  };

private:
  static xiiIdTable<xiiActionId, xiiActionDescriptor*> s_ActionTable;
  static xiiMap<xiiString, CategoryData>               s_CategoryPathToActions;
  static xiiMap<xiiString, xiiString>                  s_ShortcutOverride;
};
