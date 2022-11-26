#pragma once

#include <Core/Input/Declarations.h>
#include <Core/World/Declarations.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/GameState/GameState.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

class XII_SAMPLEGAMEPLUGIN_DLL SampleGameState : public xiiFallbackGameState
{
  XII_ADD_DYNAMIC_REFLECTION(SampleGameState, xiiFallbackGameState);

public:
  SampleGameState();

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

  // BEGIN-DOCS-CODE-SNIPPET: confunc-decl
  void                                ConFunc_Print(xiiString sText);
  xiiConsoleFunction<void(xiiString)> m_ConFunc_Print;
  // END-DOCS-CODE-SNIPPET

  xiiDeque<xiiGameObjectHandle> m_SpawnedObjects;
};
