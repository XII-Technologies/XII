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

void xiiSceneGizmoActions::MapMenuActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hGreyBoxingGizmo, "G.Gizmos", 5.0f);
}

void xiiSceneGizmoActions::MapToolbarActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  const xiiStringView sSubPath("GizmoCategory");
  pMap->MapAction(s_hGreyBoxingGizmo, sSubPath, 5.0f);
}
