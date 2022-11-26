#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class xiiGameEngineTestApplication_TypeScript : public xiiGameEngineTestApplication
{
public:
  xiiGameEngineTestApplication_TypeScript();

  void          SubTestBasicsSetup();
  xiiTestAppRun SubTestBasisExec(const char* szSubTestName);
};

class xiiGameEngineTestTypeScript : public xiiGameEngineTest
{
  using SUPER = xiiGameEngineTest;

public:
  virtual const char*                   GetTestName() const override;
  virtual xiiGameEngineTestApplication* CreateApplication() override;

  enum SubTests
  {
    Vec2,
    Vec3,
    Quat,
    Mat3,
    Mat4,
    Transform,
    Color,
    Debug,
    GameObject,
    Component,
    Lifetime,
    Messaging,
    World,
    Utils,
  };

private:
  virtual void          SetupSubTests() override;
  virtual xiiResult     InitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override;

  xiiGameEngineTestApplication_TypeScript* m_pOwnApplication = nullptr;
};
