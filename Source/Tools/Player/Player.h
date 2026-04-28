/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class xiiPlayerApplication : public xiiGameApplication
{
public:
  using SUPER = xiiGameApplication;

  xiiPlayerApplication();

protected:
  virtual void      Run_InputUpdate() override;
  virtual xiiResult BeforeCoreSystemsStartup() override;
  virtual void      AfterCoreSystemsStartup() override;

private:
  void DetermineProjectPath();
};
