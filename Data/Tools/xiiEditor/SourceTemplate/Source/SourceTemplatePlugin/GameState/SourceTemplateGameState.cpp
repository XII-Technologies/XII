#include <SourceTemplatePlugin/SourceTemplatePluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/System/Window.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Logging/Log.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Meshes/MeshComponent.h>

#include <SourceTemplatePlugin/GameState/SourceTemplateGameState.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(SourceTemplateGameState, 1, xiiRTTIDefaultAllocator<SourceTemplateGameState>)
XII_END_DYNAMIC_REFLECTED_TYPE;

SourceTemplateGameState::SourceTemplateGameState()  = default;
SourceTemplateGameState::~SourceTemplateGameState() = default;

void SourceTemplateGameState::OnActivation(xiiWorld* pWorld, const xiiTransform* pStartPosition)
{
  XII_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld, pStartPosition);
}

void SourceTemplateGameState::OnDeactivation()
{
  XII_LOG_BLOCK("GameState::Deactivate");

  SUPER::OnDeactivation();
}

void SourceTemplateGameState::AfterWorldUpdate()
{
  SUPER::AfterWorldUpdate();
}

void SourceTemplateGameState::BeforeWorldUpdate()
{
  XII_LOCK(m_pMainWorld->GetWriteMarker());
}

xiiGameStatePriority SourceTemplateGameState::DeterminePriority(xiiWorld* pWorld) const
{
  return xiiGameStatePriority::Default;
}

void SourceTemplateGameState::ConfigureMainWindowInputDevices(xiiWindow* pWindow)
{
  SUPER::ConfigureMainWindowInputDevices(pWindow);

  // Setup devices here
}

void SourceTemplateGameState::ConfigureInputActions()
{
  SUPER::ConfigureInputActions();
}

void SourceTemplateGameState::ProcessInput()
{
  SUPER::ProcessInput();

  xiiWorld* pWorld = m_pMainWorld;
}

void SourceTemplateGameState::ConfigureMainCamera()
{
  SUPER::ConfigureMainCamera();

  // Custom camera setup here
}
