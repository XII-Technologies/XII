#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorPluginParticle/Actions/ParticleActions.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <GuiFoundation/Action/ActionManager.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleAction, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActionDescriptorHandle xiiParticleActions::s_hCategory;
xiiActionDescriptorHandle xiiParticleActions::s_hPauseEffect;
xiiActionDescriptorHandle xiiParticleActions::s_hRestartEffect;
xiiActionDescriptorHandle xiiParticleActions::s_hAutoRestart;
xiiActionDescriptorHandle xiiParticleActions::s_hSimulationSpeedMenu;
xiiActionDescriptorHandle xiiParticleActions::s_hSimulationSpeed[10];
xiiActionDescriptorHandle xiiParticleActions::s_hRenderVisualizers;


void xiiParticleActions::RegisterActions()
{
  s_hCategory = XII_REGISTER_CATEGORY("ParticleCategory");
  s_hPauseEffect =
    XII_REGISTER_ACTION_1("PFX.Pause", xiiActionScope::Document, "Particles", "Pause", xiiParticleAction, xiiParticleAction::ActionType::PauseEffect);
  s_hRestartEffect =
    XII_REGISTER_ACTION_1("PFX.Restart", xiiActionScope::Document, "Particles", "F5", xiiParticleAction, xiiParticleAction::ActionType::RestartEffect);
  s_hAutoRestart =
    XII_REGISTER_ACTION_1("PFX.AutoRestart", xiiActionScope::Document, "Particles", "", xiiParticleAction, xiiParticleAction::ActionType::AutoRestart);

  s_hSimulationSpeedMenu = XII_REGISTER_MENU_WITH_ICON("PFX.Speed.Menu", ":/EditorPluginParticle/Icons/Speed16.png");
  s_hSimulationSpeed[0]  = XII_REGISTER_ACTION_2(
    "PFX.Speed.01", xiiActionScope::Document, "Particles", "Ctrl+1", xiiParticleAction, xiiParticleAction::ActionType::SimulationSpeed, 0.1f);
  s_hSimulationSpeed[1] = XII_REGISTER_ACTION_2(
    "PFX.Speed.025", xiiActionScope::Document, "Particles", "Ctrl+2", xiiParticleAction, xiiParticleAction::ActionType::SimulationSpeed, 0.25f);
  s_hSimulationSpeed[2] = XII_REGISTER_ACTION_2(
    "PFX.Speed.05", xiiActionScope::Document, "Particles", "Ctrl+3", xiiParticleAction, xiiParticleAction::ActionType::SimulationSpeed, 0.5f);
  s_hSimulationSpeed[3] = XII_REGISTER_ACTION_2(
    "PFX.Speed.1", xiiActionScope::Document, "Particles", "Ctrl+4", xiiParticleAction, xiiParticleAction::ActionType::SimulationSpeed, 1.0f);
  s_hSimulationSpeed[4] = XII_REGISTER_ACTION_2(
    "PFX.Speed.15", xiiActionScope::Document, "Particles", "Ctrl+5", xiiParticleAction, xiiParticleAction::ActionType::SimulationSpeed, 1.5f);
  s_hSimulationSpeed[5] = XII_REGISTER_ACTION_2(
    "PFX.Speed.2", xiiActionScope::Document, "Particles", "Ctrl+6", xiiParticleAction, xiiParticleAction::ActionType::SimulationSpeed, 2.0f);
  s_hSimulationSpeed[6] = XII_REGISTER_ACTION_2(
    "PFX.Speed.3", xiiActionScope::Document, "Particles", "Ctrl+7", xiiParticleAction, xiiParticleAction::ActionType::SimulationSpeed, 3.0f);
  s_hSimulationSpeed[7] = XII_REGISTER_ACTION_2(
    "PFX.Speed.4", xiiActionScope::Document, "Particles", "Ctrl+8", xiiParticleAction, xiiParticleAction::ActionType::SimulationSpeed, 4.0f);
  s_hSimulationSpeed[8] = XII_REGISTER_ACTION_2(
    "PFX.Speed.5", xiiActionScope::Document, "Particles", "Ctrl+9", xiiParticleAction, xiiParticleAction::ActionType::SimulationSpeed, 5.0f);
  s_hSimulationSpeed[9] = XII_REGISTER_ACTION_2(
    "PFX.Speed.10", xiiActionScope::Document, "Particles", "Ctrl+0", xiiParticleAction, xiiParticleAction::ActionType::SimulationSpeed, 10.0f);
  s_hRenderVisualizers = XII_REGISTER_ACTION_1(
    "PFX.Render.Visualizers", xiiActionScope::Document, "Particles", "V", xiiParticleAction, xiiParticleAction::ActionType::RenderVisualizers);
}

void xiiParticleActions::UnregisterActions()
{
  xiiActionManager::UnregisterAction(s_hCategory);
  xiiActionManager::UnregisterAction(s_hPauseEffect);
  xiiActionManager::UnregisterAction(s_hRestartEffect);
  xiiActionManager::UnregisterAction(s_hAutoRestart);
  xiiActionManager::UnregisterAction(s_hSimulationSpeedMenu);
  xiiActionManager::UnregisterAction(s_hRenderVisualizers);

  for (int i = 0; i < XII_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    xiiActionManager::UnregisterAction(s_hSimulationSpeed[i]);
}

void xiiParticleActions::MapActions(const char* szMapping, const char* szPath)
{
  xiiActionMap* pMap = xiiActionMapManager::GetActionMap(szMapping);
  XII_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "ParticleCategory";

  pMap->MapAction(s_hPauseEffect, szSubPath, 0.5f);
  pMap->MapAction(s_hRestartEffect, szSubPath, 1.0f);
  pMap->MapAction(s_hAutoRestart, szSubPath, 2.0f);

  pMap->MapAction(s_hSimulationSpeedMenu, szSubPath, 3.0f);

  xiiStringBuilder sSubPath(szSubPath, "/PFX.Speed.Menu");

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(s_hSimulationSpeed); ++i)
    pMap->MapAction(s_hSimulationSpeed[i], sSubPath, i + 1.0f);

  pMap->MapAction(s_hRenderVisualizers, szSubPath, 4.0f);
}

xiiParticleAction::xiiParticleAction(const xiiActionContext& context, const char* szName, xiiParticleAction::ActionType type, float fSimSpeed) :
  xiiButtonAction(context, szName, false, "")
{
  m_Type      = type;
  m_fSimSpeed = fSimSpeed;

  m_pEffectDocument = const_cast<xiiParticleEffectAssetDocument*>(static_cast<const xiiParticleEffectAssetDocument*>(context.m_pDocument));
  m_pEffectDocument->m_Events.AddEventHandler(xiiMakeDelegate(&xiiParticleAction::EffectEventHandler, this));

  switch (m_Type)
  {
    case ActionType::PauseEffect:
      SetIconPath(":/EditorPluginParticle/Icons/Pause16.png");
      break;

    case ActionType::RestartEffect:
      SetIconPath(":/EditorPluginParticle/Icons/Restart16.png");
      break;

    case ActionType::AutoRestart:
      SetIconPath(":/EditorPluginParticle/Icons/Loop16.png");
      break;

    case ActionType::RenderVisualizers:
      SetCheckable(true);
      SetIconPath(":/EditorFramework/Icons/Visualizers16.png");
      SetChecked(m_pEffectDocument->GetRenderVisualizers());
      break;

    default:
      break;
  }

  UpdateState();
}


xiiParticleAction::~xiiParticleAction()
{
  m_pEffectDocument->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiParticleAction::EffectEventHandler, this));
}

void xiiParticleAction::Execute(const xiiVariant& value)
{
  switch (m_Type)
  {
    case ActionType::PauseEffect:
      m_pEffectDocument->SetSimulationPaused(!m_pEffectDocument->GetSimulationPaused());
      return;

    case ActionType::RestartEffect:
      m_pEffectDocument->TriggerRestartEffect();
      return;

    case ActionType::AutoRestart:
      m_pEffectDocument->SetAutoRestart(!m_pEffectDocument->GetAutoRestart());
      return;

    case ActionType::SimulationSpeed:
      m_pEffectDocument->SetSimulationSpeed(m_fSimSpeed);
      return;

    case ActionType::RenderVisualizers:
      m_pEffectDocument->SetRenderVisualizers(!m_pEffectDocument->GetRenderVisualizers());
      return;
  }
}

void xiiParticleAction::EffectEventHandler(const xiiParticleEffectAssetEvent& e)
{
  switch (e.m_Type)
  {
    case xiiParticleEffectAssetEvent::AutoRestartChanged:
    case xiiParticleEffectAssetEvent::SimulationSpeedChanged:
    case xiiParticleEffectAssetEvent::RenderVisualizersChanged:
      UpdateState();
      break;

    default:
      break;
  }
}

void xiiParticleAction::UpdateState()
{
  if (m_Type == ActionType::PauseEffect)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetSimulationPaused());
  }

  if (m_Type == ActionType::AutoRestart)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetAutoRestart());
  }

  if (m_Type == ActionType::SimulationSpeed)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetSimulationSpeed() == m_fSimSpeed);
  }

  if (m_Type == ActionType::RenderVisualizers)
  {
    SetCheckable(true);
    SetChecked(m_pEffectDocument->GetRenderVisualizers());
  }
}
