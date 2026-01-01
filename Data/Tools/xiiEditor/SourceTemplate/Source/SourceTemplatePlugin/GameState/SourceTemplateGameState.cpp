#include <SourceTemplatePlugin/SourceTemplatePluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/System/Window.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Logging/Log.h>

#include <SourceTemplatePlugin/GameState/SourceTemplateGameState.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(SourceTemplateGameState, 1, xiiRTTIDefaultAllocator<SourceTemplateGameState>)
XII_END_DYNAMIC_REFLECTED_TYPE;

SourceTemplateGameState::SourceTemplateGameState() = default;
SourceTemplateGameState::~SourceTemplateGameState() = default;

void SourceTemplateGameState::GetStartupOptions(xiiString& out_sScene, xiiString& out_sPreloadCollection)
{
  // replace this to load a certain scene at startup
  // the default implementation looks at the command line "-scene" argument

  // if we have a "-scene" command line argument, it was launched from the editor and we should load that
  if (xiiCommandLineUtils::GetGlobalInstance()->HasOption("-scene"))
  {
    out_sScene = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene");
  }
  else
  {
    // otherwise, we use the hardcoded 'Main.xiiScene'
    // if that doesn't exist, this function has to be adjusted
    // note that you can return an asset GUID here, instead of a path
    out_sScene = "AssetCache/Common/Scenes/Main.xiiBinScene";
  }

  xiiStringBuilder sPreloadCollection = out_sScene;
  sPreloadCollection.ChangeFileExtension("xiiBinCollection");
  if (xiiFileSystem::ExistsFile(sPreloadCollection))
  {
    out_sPreloadCollection = sPreloadCollection;
  }
}

void SourceTemplateGameState::OnActivation(xiiWorld* pWorld, xiiStringView sStartPosition, const xiiTransform& startPositionOffset)
{
  XII_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld, sStartPosition, startPositionOffset);

  // the main entry point when the game starts
  // could do some setup here, but in a lot of cases it is better to leave this as is
  // and instead override the various other virtual functions that the game state provides
  // see below and see xiiGameState for additional details
}

void SourceTemplateGameState::AfterWorldUpdate()
{
  SUPER::AfterWorldUpdate();
}

void SourceTemplateGameState::BeforeWorldUpdate()
{
  SUPER::BeforeWorldUpdate();

  XII_LOCK(m_pMainWorld->GetWriteMarker());

  // if you need to modify the world, this is a good place to do it
}

xiiResult SourceTemplateGameState::SpawnPlayer(xiiStringView sStartPosition, const xiiTransform& startPositionOffset)
{
  // replace this to create a custom player object or load a prefab
  return SUPER::SpawnPlayer(sStartPosition, startPositionOffset);
}

void SourceTemplateGameState::OnChangedMainWorld(xiiWorld* pPrevWorld, xiiWorld* pNewWorld, xiiStringView sStartPosition, const xiiTransform& startPositionOffset)
{
  SUPER::OnChangedMainWorld(pPrevWorld, pNewWorld, sStartPosition, startPositionOffset);

  // called whenever the main world is changed, ie when transitioning between levels
  // may need to update references to the world here or reset some state
}

static void RegisterInputAction(const char* szInputSet, const char* szInputAction, const char* szKey1, const char* szKey2 = nullptr, const char* szKey3 = nullptr)
{
  xiiInputActionConfig cfg;
  cfg.m_bApplyTimeScaling = true;
  cfg.m_sInputSlotTrigger[0] = szKey1;
  cfg.m_sInputSlotTrigger[1] = szKey2;
  cfg.m_sInputSlotTrigger[2] = szKey3;

  xiiInputManager::SetInputActionConfig(szInputSet, szInputAction, cfg, true);
}

void SourceTemplateGameState::ConfigureInputActions()
{
  SUPER::ConfigureInputActions();
}

void SourceTemplateGameState::ProcessInput()
{
  SUPER::ProcessInput();
}

void SourceTemplateGameState::ConfigureMainCamera()
{
  SUPER::ConfigureMainCamera();

  // do custom camera setup here
}
