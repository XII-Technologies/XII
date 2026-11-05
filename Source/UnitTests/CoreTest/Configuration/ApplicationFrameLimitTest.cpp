/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>

namespace
{
  class FrameLimitApplication final : public xiiGameApplicationBase
  {
  public:
    FrameLimitApplication() : xiiGameApplicationBase("Frame limit test") {}
    xiiString FindProjectDirectory() const override { return {}; }
    void      Init_SetupGraphicsDevice() override {}
    void      Deinit_ShutdownGraphicsDevice() override {}
    void      Run_InputUpdate() override {}
    void      Run_WorldUpdateAndRender() override {}
  };
} // namespace

XII_CREATE_SIMPLE_TEST(Configuration, ApplicationFrameLimit)
{
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(xiiStartup::ShutdownHighLevelSystems());
  auto* limit = static_cast<xiiCVarInt*>(xiiCVar::FindCVarByName("App.FrameLimit"));
  if (!XII_TEST_BOOL(limit != nullptr))
    return;
  const auto previous = limit->GetValue();
  XII_SCOPE_EXIT(*limit = previous);
  FrameLimitApplication application;
  *limit = 50;
  application.RunOneFrame();
  const auto start = xiiTime::Now();
  for (xiiUInt32 i = 0; i < 3; ++i)
    application.RunOneFrame();
  XII_TEST_BOOL((xiiTime::Now() - start).GetMilliseconds() >= 55.0);
  XII_TEST_BOOL(application.GetFrameTime().GetMilliseconds() >= 18.0);
  *limit = 0;
  application.RunOneFrame();
  XII_TEST_INT(limit->GetValue(), 0);
  *limit = -1;
  application.RunOneFrame();
  XII_TEST_INT(limit->GetValue(), 0);
  *limit = 1001;
  application.RunOneFrame();
  XII_TEST_INT(limit->GetValue(), 1000);
}
