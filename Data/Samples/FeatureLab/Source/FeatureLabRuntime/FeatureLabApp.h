#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class FeatureLabApp : public xiiGameApplication
{
public:
  using SUPER = xiiGameApplication;

  FeatureLabApp();

protected:
  virtual void Run_InputUpdate() override;
  virtual xiiResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual xiiUniquePtr<xiiGameStateBase> CreateGameState() override;

private:
  xiiResult TryProjectFolder(xiiStringView sPath);
  void DetermineProjectPath();
};
