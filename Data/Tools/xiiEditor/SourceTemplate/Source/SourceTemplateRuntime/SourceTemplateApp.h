#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class SourceTemplateApp : public xiiGameApplication
{
public:
  using SUPER = xiiGameApplication;

  SourceTemplateApp();

protected:
  virtual void Run_InputUpdate() override;
  virtual xiiResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual xiiUniquePtr<xiiGameStateBase> CreateGameState() override;

private:
  xiiResult TryProjectFolder(xiiStringView sPath);
  void DetermineProjectPath();
};
