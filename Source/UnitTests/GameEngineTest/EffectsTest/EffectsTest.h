#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class xiiGameEngineTestEffects : public xiiGameEngineTest
{
  using SUPER = xiiGameEngineTest;

public:
  virtual const char*                   GetTestName() const override;
  virtual xiiGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    Decals,
    Heightfield,
    WindClothRopes,
    Reflections,
    StressTest
  };

  virtual void          SetupSubTests() override;
  virtual xiiResult     InitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override;

  xiiInt32                      m_iFrame          = 0;
  xiiGameEngineTestApplication* m_pOwnApplication = nullptr;

  xiiUInt32 m_uiImgCompIdx = 0;

  struct ImgCompare
  {
    ImgCompare(xiiUInt32 uiFrame, xiiUInt32 uiThreshold = 450)
    {
      m_uiFrame     = uiFrame;
      m_uiThreshold = uiThreshold;
    }

    xiiUInt32 m_uiFrame;
    xiiUInt32 m_uiThreshold = 450;
  };

  xiiHybridArray<ImgCompare, 8> m_ImgCompFrames;
};
