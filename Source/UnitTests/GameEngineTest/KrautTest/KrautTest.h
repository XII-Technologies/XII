#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

class xiiGameEngineTestKraut : public xiiGameEngineTest
{
  using SUPER = xiiGameEngineTest;

public:
  virtual const char*                   GetTestName() const override;
  virtual xiiGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    TreeRendering,
  };

  virtual void          SetupSubTests() override;
  virtual xiiResult     InitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override;

  xiiInt32                      m_iFrame          = 0;
  xiiGameEngineTestApplication* m_pOwnApplication = nullptr;

  xiiUInt32                    m_uiImgCompIdx = 0;
  xiiHybridArray<xiiUInt32, 8> m_ImgCompFrames;
};

#endif
