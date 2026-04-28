/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/CommonAssetActions.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCommonAssetAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActionDescriptorHandle xiiCommonAssetActions::s_hCategory;
xiiActionDescriptorHandle xiiCommonAssetActions::s_hPause;
xiiActionDescriptorHandle xiiCommonAssetActions::s_hRestart;
xiiActionDescriptorHandle xiiCommonAssetActions::s_hLoop;
xiiActionDescriptorHandle xiiCommonAssetActions::s_hSimulationSpeedMenu;
xiiActionDescriptorHandle xiiCommonAssetActions::s_hSimulationSpeed[10];
xiiActionDescriptorHandle xiiCommonAssetActions::s_hGrid;
xiiActionDescriptorHandle xiiCommonAssetActions::s_hVisualizers;


void xiiCommonAssetActions::RegisterActions()
{
  s_hCategory    = XII_REGISTER_CATEGORY("CommonAssetCategory");
  s_hPause       = XII_REGISTER_ACTION_1("Common.Pause", xiiActionScope::Document, "Animations", "Pause", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::Pause);
  s_hRestart     = XII_REGISTER_ACTION_1("Common.Restart", xiiActionScope::Document, "Animations", "F5", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::Restart);
  s_hLoop        = XII_REGISTER_ACTION_1("Common.Loop", xiiActionScope::Document, "Animations", "", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::Loop);
  s_hGrid        = XII_REGISTER_ACTION_1("Common.Grid", xiiActionScope::Document, "Misc", "G", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::Grid);
  s_hVisualizers = XII_REGISTER_ACTION_1("Common.Visualizers", xiiActionScope::Document, "Misc", "V", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::Visualizers);

  s_hSimulationSpeedMenu = XII_REGISTER_MENU_WITH_ICON("Common.Speed.Menu", ":/EditorPluginParticle/Icons/Speed.svg");
  s_hSimulationSpeed[0]  = XII_REGISTER_ACTION_2("Common.Speed.01", xiiActionScope::Document, "Animations", "Ctrl+1", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::SimulationSpeed, 0.1f);
  s_hSimulationSpeed[1]  = XII_REGISTER_ACTION_2("Common.Speed.025", xiiActionScope::Document, "Animations", "Ctrl+2", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::SimulationSpeed, 0.25f);
  s_hSimulationSpeed[2]  = XII_REGISTER_ACTION_2("Common.Speed.05", xiiActionScope::Document, "Animations", "Ctrl+3", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::SimulationSpeed, 0.5f);
  s_hSimulationSpeed[3]  = XII_REGISTER_ACTION_2("Common.Speed.1", xiiActionScope::Document, "Animations", "Ctrl+4", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::SimulationSpeed, 1.0f);
  s_hSimulationSpeed[4]  = XII_REGISTER_ACTION_2("Common.Speed.15", xiiActionScope::Document, "Animations", "Ctrl+5", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::SimulationSpeed, 1.5f);
  s_hSimulationSpeed[5]  = XII_REGISTER_ACTION_2("Common.Speed.2", xiiActionScope::Document, "Animations", "Ctrl+6", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::SimulationSpeed, 2.0f);
  s_hSimulationSpeed[6]  = XII_REGISTER_ACTION_2("Common.Speed.3", xiiActionScope::Document, "Animations", "Ctrl+7", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::SimulationSpeed, 3.0f);
  s_hSimulationSpeed[7]  = XII_REGISTER_ACTION_2("Common.Speed.4", xiiActionScope::Document, "Animations", "Ctrl+8", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::SimulationSpeed, 4.0f);
  s_hSimulationSpeed[8]  = XII_REGISTER_ACTION_2("Common.Speed.5", xiiActionScope::Document, "Animations", "Ctrl+9", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::SimulationSpeed, 5.0f);
  s_hSimulationSpeed[9]  = XII_REGISTER_ACTION_2("Common.Speed.10", xiiActionScope::Document, "Animations", "Ctrl+0", xiiCommonAssetAction, xiiCommonAssetAction::ActionType::SimulationSpeed, 10.0f);
}

void xiiCommonAssetActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hCategory);
  xiiActionManager::UnregisterAction(s_hPause);
  xiiActionManager::UnregisterAction(s_hRestart);
  xiiActionManager::UnregisterAction(s_hLoop);
  xiiActionManager::UnregisterAction(s_hSimulationSpeedMenu);
  xiiActionManager::UnregisterAction(s_hGrid);
  xiiActionManager::UnregisterAction(s_hVisualizers);

  for (int i = 0; i < XII_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    xiiActionManager::UnregisterAction(s_hSimulationSpeed[i]);
}

void xiiCommonAssetActions::MapToolbarActions(xiiStringView sMapping, xiiUInt32 uiStateMask)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(sMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "CommonAssetCategory";

  if (uiStateMask & xiiCommonAssetUiState::Pause)
  {
    pMap->MapAction(s_hPause, szSubPath, 0.5f);
  }

  if (uiStateMask & xiiCommonAssetUiState::Restart)
  {
    pMap->MapAction(s_hRestart, szSubPath, 1.0f);
  }

  if (uiStateMask & xiiCommonAssetUiState::Loop)
  {
    pMap->MapAction(s_hLoop, szSubPath, 2.0f);
  }

  if (uiStateMask & xiiCommonAssetUiState::SimulationSpeed)
  {
    pMap->MapAction(s_hSimulationSpeedMenu, szSubPath, 3.0f);

    xiiStringBuilder sSubPath(szSubPath, "/Common.Speed.Menu");

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    {
      pMap->MapAction(s_hSimulationSpeed[i], sSubPath, i + 1.0f);
    }
  }

  if (uiStateMask & xiiCommonAssetUiState::Grid)
  {
    pMap->MapAction(s_hGrid, szSubPath, 4.0f);
  }

  if (uiStateMask & xiiCommonAssetUiState::Visualizers)
  {
    pMap->MapAction(s_hVisualizers, szSubPath, 5.0f);
  }
}

xiiCommonAssetAction::xiiCommonAssetAction(const xiiActionContext& context, const char* szName, xiiCommonAssetAction::ActionType type, float fSimSpeed) :
  xiiButtonAction(context, szName, false, "")
{
  m_Type      = type;
  m_fSimSpeed = fSimSpeed;

  m_pAssetDocument = const_cast<xiiAssetDocument*>(static_cast<const xiiAssetDocument*>(context.m_pDocument));
  m_pAssetDocument->m_CommonAssetUiChangeEvent.AddEventHandler(xiiMakeDelegate(&xiiCommonAssetAction::CommonUiEventHandler, this));

  switch (m_Type)
  {
    case ActionType::Pause:
      SetCheckable(true);
      SetIconPath(":/EditorPluginParticle/Icons/Pause.svg");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(xiiCommonAssetUiState::Pause) != 0.0f);
      break;

    case ActionType::Restart:
      SetIconPath(":/EditorPluginParticle/Icons/Restart.svg");
      break;

    case ActionType::Loop:
      SetCheckable(true);
      SetIconPath(":/EditorPluginParticle/Icons/Loop.svg");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(xiiCommonAssetUiState::Loop) != 0.0f);
      break;

    case ActionType::Grid:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Grid.svg");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(xiiCommonAssetUiState::Grid) != 0.0f);
      break;

    case ActionType::Visualizers:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Visualizers.svg");
      SetChecked(m_pAssetDocument->GetCommonAssetUiState(xiiCommonAssetUiState::Visualizers) != 0.0f);
      break;

    default:
      break;
  }

  UpdateState();
}


xiiCommonAssetAction::~xiiCommonAssetAction()
{
  m_pAssetDocument->m_CommonAssetUiChangeEvent.RemoveEventHandler(xiiMakeDelegate(&xiiCommonAssetAction::CommonUiEventHandler, this));
}

void xiiCommonAssetAction::Execute(const xiiVariant& value)
{
  switch (m_Type)
  {
    case ActionType::Pause:
      m_pAssetDocument->SetCommonAssetUiState(xiiCommonAssetUiState::Pause, m_pAssetDocument->GetCommonAssetUiState(xiiCommonAssetUiState::Pause) == 0.0f ? 1.0f : 0.0f);
      return;

    case ActionType::Restart:
      m_pAssetDocument->SetCommonAssetUiState(xiiCommonAssetUiState::Restart, 1.0f);
      return;

    case ActionType::Loop:
      m_pAssetDocument->SetCommonAssetUiState(xiiCommonAssetUiState::Loop, m_pAssetDocument->GetCommonAssetUiState(xiiCommonAssetUiState::Loop) == 0.0f ? 1.0f : 0.0f);
      return;

    case ActionType::SimulationSpeed:
      m_pAssetDocument->SetCommonAssetUiState(xiiCommonAssetUiState::SimulationSpeed, m_fSimSpeed);
      return;

    case ActionType::Grid:
      m_pAssetDocument->SetCommonAssetUiState(xiiCommonAssetUiState::Grid, m_pAssetDocument->GetCommonAssetUiState(xiiCommonAssetUiState::Grid) == 0.0f ? 1.0f : 0.0f);
      return;

    case ActionType::Visualizers:
      m_pAssetDocument->SetCommonAssetUiState(xiiCommonAssetUiState::Visualizers, m_pAssetDocument->GetCommonAssetUiState(xiiCommonAssetUiState::Visualizers) == 0.0f ? 1.0f : 0.0f);
      return;
  }
}

void xiiCommonAssetAction::CommonUiEventHandler(const xiiCommonAssetUiState& e)
{
  if (e.m_State == xiiCommonAssetUiState::Loop || e.m_State == xiiCommonAssetUiState::SimulationSpeed || e.m_State == xiiCommonAssetUiState::Grid || e.m_State == xiiCommonAssetUiState::Visualizers)
  {
    UpdateState();
  }
}

void xiiCommonAssetAction::UpdateState()
{
  if (m_Type == ActionType::Pause)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(xiiCommonAssetUiState::Pause) != 0.0f);
  }

  if (m_Type == ActionType::Loop)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(xiiCommonAssetUiState::Loop) != 0.0f);
  }

  if (m_Type == ActionType::SimulationSpeed)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(xiiCommonAssetUiState::SimulationSpeed) == m_fSimSpeed);
  }

  if (m_Type == ActionType::Grid)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(xiiCommonAssetUiState::Grid) != 0.0f);
  }

  if (m_Type == ActionType::Visualizers)
  {
    SetCheckable(true);
    SetChecked(m_pAssetDocument->GetCommonAssetUiState(xiiCommonAssetUiState::Visualizers) != 0.0f);
  }
}
