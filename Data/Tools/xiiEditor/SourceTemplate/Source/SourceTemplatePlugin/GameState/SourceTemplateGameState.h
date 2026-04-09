#pragma once

#include <SourceTemplatePlugin/SourceTemplatePluginDLL.h>

#include <Core/Input/Declarations.h>
#include <Core/World/Declarations.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/GameState.h>

class SourceTemplateGameState : public xiiGameState
{
  XII_ADD_DYNAMIC_REFLECTION(SourceTemplateGameState, xiiGameState);

public:
  SourceTemplateGameState();
  ~SourceTemplateGameState();

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureInputActions() override;
  virtual void ConfigureMainCamera() override;
  virtual xiiResult SpawnPlayer(xiiStringView sStartPosition, const xiiTransform& startPositionOffset) override;
  virtual void OnChangedMainWorld(xiiWorld* pPrevWorld, xiiWorld* pNewWorld, xiiStringView sStartPosition, const xiiTransform& startPositionOffset) override;
  virtual void GetStartupOptions(xiiString& out_sScene, xiiString& out_sPreloadCollection) override;

private:
  virtual void OnActivation(xiiWorld* pWorld, xiiStringView sStartPosition, const xiiTransform& startPositionOffset) override;
  virtual void BeforeWorldUpdate() override;
  virtual void AfterWorldUpdate() override;
};
