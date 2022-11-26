#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class xiiGameEngineTestApplication_Basics : public xiiGameEngineTestApplication
{
public:
  xiiGameEngineTestApplication_Basics();

  void          SubTestManyMeshesSetup();
  xiiTestAppRun SubTestManyMeshesExec(xiiInt32 iCurFrame);

  void          SubTestSkyboxSetup();
  xiiTestAppRun SubTestSkyboxExec(xiiInt32 iCurFrame);

  void          SubTestDebugRenderingSetup();
  xiiTestAppRun SubTestDebugRenderingExec(xiiInt32 iCurFrame);

  xiiTestAppRun SubTestDebugRenderingExec2(xiiInt32 iCurFrame);

  void          SubTestLoadSceneSetup();
  xiiTestAppRun SubTestLoadSceneExec(xiiInt32 iCurFrame);
};

class xiiGameEngineTestBasics : public xiiGameEngineTest
{
  using SUPER = xiiGameEngineTest;

public:
  virtual const char*                   GetTestName() const override;
  virtual xiiGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    ManyMeshes,
    Skybox,
    DebugRendering,
    DebugRendering2,
    LoadScene,
  };

  virtual void          SetupSubTests() override;
  virtual xiiResult     InitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override;

  xiiInt32                             m_iFrame;
  xiiGameEngineTestApplication_Basics* m_pOwnApplication;
};
