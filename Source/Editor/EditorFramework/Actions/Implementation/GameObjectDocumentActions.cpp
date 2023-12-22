#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/Preferences/ScenePreferences.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameObjectDocumentAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCameraSpeedSliderAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiActionDescriptorHandle xiiGameObjectDocumentActions::s_hGameObjectCategory;
xiiActionDescriptorHandle xiiGameObjectDocumentActions::s_hRenderSelectionOverlay;
xiiActionDescriptorHandle xiiGameObjectDocumentActions::s_hRenderVisualizers;
xiiActionDescriptorHandle xiiGameObjectDocumentActions::s_hRenderShapeIcons;
xiiActionDescriptorHandle xiiGameObjectDocumentActions::s_hRenderGrid;
xiiActionDescriptorHandle xiiGameObjectDocumentActions::s_hAddAmbientLight;
xiiActionDescriptorHandle xiiGameObjectDocumentActions::s_hSimulationSpeedMenu;
xiiActionDescriptorHandle xiiGameObjectDocumentActions::s_hSimulationSpeed[10];
xiiActionDescriptorHandle xiiGameObjectDocumentActions::s_hCameraSpeed;
xiiActionDescriptorHandle xiiGameObjectDocumentActions::s_hPickTransparent;

void xiiGameObjectDocumentActions::RegisterActions()
{
  s_hGameObjectCategory     = XII_REGISTER_CATEGORY("GameObjectCategory");
  s_hRenderSelectionOverlay = XII_REGISTER_ACTION_1("Scene.Render.SelectionOverlay", xiiActionScope::Document, "Scene", "S", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::RenderSelectionOverlay);
  s_hRenderVisualizers      = XII_REGISTER_ACTION_1("Scene.Render.Visualizers", xiiActionScope::Document, "Scene", "V", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::RenderVisualizers);
  s_hRenderShapeIcons       = XII_REGISTER_ACTION_1("Scene.Render.ShapeIcons", xiiActionScope::Document, "Scene", "I", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::RenderShapeIcons);
  s_hRenderGrid             = XII_REGISTER_ACTION_1("Scene.Render.Grid", xiiActionScope::Document, "Scene", "G", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::RenderGrid);
  s_hAddAmbientLight        = XII_REGISTER_ACTION_1("Scene.Render.AddAmbient", xiiActionScope::Document, "Scene", "", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::AddAmbientLight);

  s_hSimulationSpeedMenu = XII_REGISTER_MENU_WITH_ICON("Scene.Simulation.Speed.Menu", "");
  s_hSimulationSpeed[0]  = XII_REGISTER_ACTION_2("Scene.Simulation.Speed.01", xiiActionScope::Document, "Simulation - Speed", "", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::SimulationSpeed, 0.1f);
  s_hSimulationSpeed[1]  = XII_REGISTER_ACTION_2("Scene.Simulation.Speed.025", xiiActionScope::Document, "Simulation - Speed", "", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::SimulationSpeed, 0.25f);
  s_hSimulationSpeed[2]  = XII_REGISTER_ACTION_2("Scene.Simulation.Speed.05", xiiActionScope::Document, "Simulation - Speed", "", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::SimulationSpeed, 0.5f);
  s_hSimulationSpeed[3]  = XII_REGISTER_ACTION_2("Scene.Simulation.Speed.1", xiiActionScope::Document, "Simulation - Speed", "", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::SimulationSpeed, 1.0f);
  s_hSimulationSpeed[4]  = XII_REGISTER_ACTION_2("Scene.Simulation.Speed.15", xiiActionScope::Document, "Simulation - Speed", "", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::SimulationSpeed, 1.5f);
  s_hSimulationSpeed[5]  = XII_REGISTER_ACTION_2("Scene.Simulation.Speed.2", xiiActionScope::Document, "Simulation - Speed", "", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::SimulationSpeed, 2.0f);
  s_hSimulationSpeed[6]  = XII_REGISTER_ACTION_2("Scene.Simulation.Speed.3", xiiActionScope::Document, "Simulation - Speed", "", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::SimulationSpeed, 3.0f);
  s_hSimulationSpeed[7]  = XII_REGISTER_ACTION_2("Scene.Simulation.Speed.4", xiiActionScope::Document, "Simulation - Speed", "", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::SimulationSpeed, 4.0f);
  s_hSimulationSpeed[8]  = XII_REGISTER_ACTION_2("Scene.Simulation.Speed.5", xiiActionScope::Document, "Simulation - Speed", "", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::SimulationSpeed, 5.0f);
  s_hSimulationSpeed[9]  = XII_REGISTER_ACTION_2("Scene.Simulation.Speed.10", xiiActionScope::Document, "Simulation - Speed", "", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::SimulationSpeed, 10.0f);

  s_hCameraSpeed = XII_REGISTER_ACTION_1("Scene.Camera.Speed", xiiActionScope::Document, "Camera", "", xiiCameraSpeedSliderAction, xiiCameraSpeedSliderAction::ActionType::CameraSpeed);

  s_hPickTransparent = XII_REGISTER_ACTION_1("Scene.Render.PickTransparent", xiiActionScope::Document, "Scene", "U", xiiGameObjectDocumentAction, xiiGameObjectDocumentAction::ActionType::PickTransparent);
}

void xiiGameObjectDocumentActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hGameObjectCategory);
  xiiActionManager::UnregisterAction(s_hRenderSelectionOverlay);
  xiiActionManager::UnregisterAction(s_hRenderVisualizers);
  xiiActionManager::UnregisterAction(s_hRenderShapeIcons);
  xiiActionManager::UnregisterAction(s_hRenderGrid);
  xiiActionManager::UnregisterAction(s_hAddAmbientLight);

  xiiActionManager::UnregisterAction(s_hSimulationSpeedMenu);
  for (int i = 0; i < XII_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    xiiActionManager::UnregisterAction(s_hSimulationSpeed[i]);

  xiiActionManager::UnregisterAction(s_hCameraSpeed);
  xiiActionManager::UnregisterAction(s_hPickTransparent);
}

void xiiGameObjectDocumentActions::MapMenuActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    pMap->MapAction(s_hGameObjectCategory, "G.View", 0.9f);

    const xiiStringView sSubPath = "GameObjectCategory";
    pMap->MapAction(s_hRenderSelectionOverlay, sSubPath, 1.0f);
    pMap->MapAction(s_hRenderVisualizers, sSubPath, 2.0f);
    pMap->MapAction(s_hRenderShapeIcons, sSubPath, 3.0f);
    pMap->MapAction(s_hRenderGrid, sSubPath, 4.0f);
    pMap->MapAction(s_hPickTransparent, sSubPath, 5.0f);
    pMap->MapAction(s_hAddAmbientLight, sSubPath, 6.0f);
    pMap->MapAction(s_hCameraSpeed, sSubPath, 7.0f);
  }
}

void xiiGameObjectDocumentActions::MapMenuSimulationSpeed(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    const xiiStringView sSubPath = "GameObjectCategory";

    pMap->MapAction(s_hGameObjectCategory, "G.Scene", 1.0f);
    pMap->MapAction(s_hSimulationSpeedMenu, sSubPath, 3.0f);

    xiiStringBuilder sSubPathSim(sSubPath, "/Scene.Simulation.Speed.Menu");
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(s_hSimulationSpeed); ++i)
      pMap->MapAction(s_hSimulationSpeed[i], "G.Scene", sSubPathSim, i + 1.0f);
  }
}

void xiiGameObjectDocumentActions::MapToolbarActions(xiiStringView sMapping)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  {
    pMap->MapAction(s_hGameObjectCategory, "", 12.0f);

    const xiiStringView sSubPath("GameObjectCategory");
    pMap->MapAction(s_hRenderSelectionOverlay, sSubPath, 4.0f);
    pMap->MapAction(s_hRenderVisualizers, sSubPath, 5.0f);
    pMap->MapAction(s_hRenderShapeIcons, sSubPath, 6.0f);
    pMap->MapAction(s_hRenderGrid, sSubPath, 6.5f);
    pMap->MapAction(s_hCameraSpeed, sSubPath, 7.0f);
  }
}

xiiGameObjectDocumentAction::xiiGameObjectDocumentAction(const xiiActionContext&                 context,const char*                             szName,xiiGameObjectDocumentAction::ActionType type,float                                   fSimSpeed) :xiiButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pGameObjectDocument = const_cast<xiiGameObjectDocument*>(static_cast<const xiiGameObjectDocument*>(context.m_pDocument));
  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(xiiMakeDelegate(&xiiGameObjectDocumentAction::SceneEventHandler, this));
  m_fSimSpeed = fSimSpeed;

  switch (m_Type)
  {
    case ActionType::RenderSelectionOverlay:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Selection.svg");
      SetChecked(m_pGameObjectDocument->GetRenderSelectionOverlay());
      break;

    case ActionType::RenderVisualizers:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Visualizers.svg");
      SetChecked(m_pGameObjectDocument->GetRenderVisualizers());
      break;

    case ActionType::RenderShapeIcons:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/ShapeIcons.svg");
      SetChecked(m_pGameObjectDocument->GetRenderShapeIcons());
      break;

    case ActionType::RenderGrid:
    {
      xiiScenePreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(m_pGameObjectDocument);
      pPreferences->m_ChangedEvent.AddEventHandler(xiiMakeDelegate(&xiiGameObjectDocumentAction::OnPreferenceChange, this));

      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Grid.svg");
      SetChecked(pPreferences->GetShowGrid());
    }
    break;

    case ActionType::AddAmbientLight:
      SetCheckable(true);
      // SetIconPath(":/EditorPluginScene/Icons/ShapeIcons.svg"); // TODO icon
      SetChecked(m_pGameObjectDocument->GetAddAmbientLight());
      break;

    case ActionType::SimulationSpeed:
      SetCheckable(true);
      SetChecked(m_pGameObjectDocument->GetSimulationSpeed() == m_fSimSpeed);
      break;

    case ActionType::PickTransparent:
      SetCheckable(true);
      // SetIconPath(":/EditorFramework/Icons/Visualizers.svg"); // TODO icon
      SetChecked(m_pGameObjectDocument->GetPickTransparent());
      break;
  }
}

xiiGameObjectDocumentAction::~xiiGameObjectDocumentAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(xiiMakeDelegate(&xiiGameObjectDocumentAction::SceneEventHandler, this));

  switch (m_Type)
  {
    case ActionType::RenderGrid:
    {
      xiiScenePreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->m_ChangedEvent.RemoveEventHandler(xiiMakeDelegate(&xiiGameObjectDocumentAction::OnPreferenceChange, this));
    }
    break;
    default:
      break;
  }
}

void xiiGameObjectDocumentAction::Execute(const xiiVariant& value)
{
  switch (m_Type)
  {
    case ActionType::RenderSelectionOverlay:
      m_pGameObjectDocument->SetRenderSelectionOverlay(!m_pGameObjectDocument->GetRenderSelectionOverlay());
      return;

    case ActionType::RenderVisualizers:
      m_pGameObjectDocument->SetRenderVisualizers(!m_pGameObjectDocument->GetRenderVisualizers());
      return;

    case ActionType::RenderShapeIcons:
      m_pGameObjectDocument->SetRenderShapeIcons(!m_pGameObjectDocument->GetRenderShapeIcons());
      return;

    case ActionType::RenderGrid:
    {
      auto pPref = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(m_pGameObjectDocument);
      pPref->SetShowGrid(!pPref->GetShowGrid());
      m_pGameObjectDocument->ShowDocumentStatus(xiiFmt("Show Grid: {}", pPref->GetShowGrid() ? "ON" : "OFF"));
      return;
    }

    case ActionType::AddAmbientLight:
      m_pGameObjectDocument->SetAddAmbientLight(!m_pGameObjectDocument->GetAddAmbientLight());
      return;

    case ActionType::SimulationSpeed:
      m_pGameObjectDocument->SetSimulationSpeed(m_fSimSpeed);
      return;

    case ActionType::PickTransparent:
      m_pGameObjectDocument->SetPickTransparent(!m_pGameObjectDocument->GetPickTransparent());
      return;

    default:
      break;
  }
}

void xiiGameObjectDocumentAction::SceneEventHandler(const xiiGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiGameObjectEvent::Type::RenderSelectionOverlayChanged:
    {
      if (m_Type == ActionType::RenderSelectionOverlay)
      {
        SetChecked(m_pGameObjectDocument->GetRenderSelectionOverlay());
      }
    }
    break;

    case xiiGameObjectEvent::Type::RenderVisualizersChanged:
    {
      if (m_Type == ActionType::RenderVisualizers)
      {
        SetChecked(m_pGameObjectDocument->GetRenderVisualizers());
      }
    }
    break;

    case xiiGameObjectEvent::Type::RenderShapeIconsChanged:
    {
      if (m_Type == ActionType::RenderShapeIcons)
      {
        SetChecked(m_pGameObjectDocument->GetRenderShapeIcons());
      }
    }
    break;

    case xiiGameObjectEvent::Type::AddAmbientLightChanged:
    {
      if (m_Type == ActionType::AddAmbientLight)
      {
        SetChecked(m_pGameObjectDocument->GetAddAmbientLight());
      }
    }
    break;

    case xiiGameObjectEvent::Type::SimulationSpeedChanged:
    {
      if (m_Type == ActionType::SimulationSpeed)
      {
        SetChecked(m_pGameObjectDocument->GetSimulationSpeed() == m_fSimSpeed);
      }
    }
    break;

    case xiiGameObjectEvent::Type::PickTransparentChanged:
    {
      if (m_Type == ActionType::PickTransparent)
      {
        SetChecked(m_pGameObjectDocument->GetPickTransparent());
      }
    }
    break;

    default:
      break;
  }
}

void xiiGameObjectDocumentAction::OnPreferenceChange(xiiPreferences* pref)
{
  xiiScenePreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(m_pGameObjectDocument);

  switch (m_Type)
  {
    case ActionType::RenderGrid:
    {
      SetChecked(pPreferences->GetShowGrid());
    }
    break;

    default:
      break;
  }
}

xiiCameraSpeedSliderAction::xiiCameraSpeedSliderAction(const xiiActionContext& context, const char* szName, ActionType type) :
  xiiSliderAction(context, szName)
{
  m_Type                = type;
  m_pGameObjectDocument = const_cast<xiiGameObjectDocument*>(static_cast<const xiiGameObjectDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      xiiScenePreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->m_ChangedEvent.AddEventHandler(xiiMakeDelegate(&xiiCameraSpeedSliderAction::OnPreferenceChange, this));

      SetRange(0, 24);
    }
    break;
  }

  UpdateState();
}

xiiCameraSpeedSliderAction::~xiiCameraSpeedSliderAction()
{
  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      xiiScenePreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->m_ChangedEvent.RemoveEventHandler(xiiMakeDelegate(&xiiCameraSpeedSliderAction::OnPreferenceChange, this));
    }
    break;
  }
}

void xiiCameraSpeedSliderAction::Execute(const xiiVariant& value)
{
  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      xiiScenePreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(m_pGameObjectDocument);

      pPreferences->SetCameraSpeed(value.Get<xiiInt32>());
    }
    break;
  }
}

void xiiCameraSpeedSliderAction::OnPreferenceChange(xiiPreferences* pref)
{
  UpdateState();
}

void xiiCameraSpeedSliderAction::UpdateState()
{
  switch (m_Type)
  {
    case ActionType::CameraSpeed:
    {
      xiiScenePreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(m_pGameObjectDocument);

      SetValue(pPreferences->GetCameraSpeed());
    }
    break;
  }
}
