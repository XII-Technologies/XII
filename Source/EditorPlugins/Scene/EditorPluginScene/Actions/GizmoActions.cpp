#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <EditorPluginScene/EditTools/GreyBoxEditTool.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

xiiActionDescriptorHandle xiiSceneGizmoActions::s_hGreyBoxingGizmo;

void xiiSceneGizmoActions::RegisterActions()
{
  s_hGreyBoxingGizmo = XII_REGISTER_ACTION_1("Gizmo.Mode.GreyBoxing", xiiActionScope::Document, "Gizmo", "B", xiiGizmoAction, xiiGetStaticRTTI<xiiGreyBoxEditTool>());
}

void xiiSceneGizmoActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hGreyBoxingGizmo);
}

void xiiSceneGizmoActions::MapMenuActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  xiiStringBuilder sSubPath(szPath, "/Gizmo.Menu");
  pMap->MapAction(s_hGreyBoxingGizmo, sSubPath, 5.0f);
}

void xiiSceneGizmoActions::MapToolbarActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  xiiStringBuilder sSubPath(szPath, "/GizmoCategory");
  pMap->MapAction(s_hGreyBoxingGizmo, sSubPath, 5.0f);
}
