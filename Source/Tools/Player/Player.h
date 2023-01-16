#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class xiiPlayerApplication : public xiiGameApplication
{
public:
  typedef xiiGameApplication SUPER;

  xiiPlayerApplication();

protected:
  virtual void      Run_InputUpdate() override;
  virtual xiiResult BeforeCoreSystemsStartup() override;
  virtual void      AfterCoreSystemsStartup() override;

private:
  void DetermineProjectPath();
};
