#pragma once

#include <Core/Input/Declarations.h>
#include <Core/World/Declarations.h>
#include <SourceTemplatePlugin/SourceTemplatePluginDLL.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/GameState/GameState.h>

class SourceTemplateGameState : public xiiFallbackGameState
{
  XII_ADD_DYNAMIC_REFLECTION(SourceTemplateGameState, xiiFallbackGameState);

public:
  SourceTemplateGameState();
  ~SourceTemplateGameState();

  virtual xiiGameStatePriority DeterminePriority(xiiWorld* pWorld) const override;

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureMainWindowInputDevices(xiiWindow* pWindow) override;
  virtual void ConfigureInputActions() override;
  virtual void ConfigureMainCamera() override;

private:
  virtual void OnActivation(xiiWorld* pWorld, const xiiTransform* pStartPosition) override;
  virtual void OnDeactivation() override;
  virtual void BeforeWorldUpdate() override;
  virtual void AfterWorldUpdate() override;
};
