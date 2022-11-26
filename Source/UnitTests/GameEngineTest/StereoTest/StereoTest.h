#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include <GameEngineTest/TestClass/TestClass.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

class xiiStereoTestGameState : public xiiGameEngineTestGameState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStereoTestGameState, xiiGameEngineTestGameState);

public:
  void OverrideRenderPipeline(xiiTypedResourceHandle<xiiRenderPipelineResource> hPipeline);
};

class xiiStereoTestApplication : public xiiGameEngineTestApplication
{
public:
  typedef xiiGameEngineTestApplication SUPER;

  xiiStereoTestApplication(const char* szProjectDirName);
  xiiPlatformProfile& GetPlatformProfile() { return m_PlatformProfile; }

protected:
  virtual xiiUniquePtr<xiiGameStateBase> CreateGameState(xiiWorld* pWorld) override;
};


class xiiStereoTest : public xiiGameEngineTest
{
  using SUPER = xiiGameEngineTest;

public:
  virtual const char*                   GetTestName() const override;
  virtual xiiGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    HoloLensPipeline,
    DefaultPipeline
  };

  virtual void          SetupSubTests() override;
  virtual xiiResult     InitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override;

  xiiInt32                  m_iFrame          = 0;
  xiiStereoTestApplication* m_pOwnApplication = nullptr;

  xiiUInt32                    m_uiImgCompIdx = 0;
  xiiHybridArray<xiiUInt32, 8> m_ImgCompFrames;
};

#endif
