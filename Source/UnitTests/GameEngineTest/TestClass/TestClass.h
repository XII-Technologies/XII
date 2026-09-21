/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <TestFramework/Framework/TestBaseClass.h>
#include <Texture/Image/Image.h>

class xiiGameEngineTestGameState : public xiiGameState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameEngineTestGameState, xiiGameState);

public:
  virtual void ProcessInput() override;
  virtual void ConfigureInputActions() override;
};

class xiiGameEngineTestApplication : public xiiGameApplication
{
public:
  using SUPER = xiiGameApplication;

  xiiGameEngineTestApplication(xiiStringView sProjectDirName);

  virtual xiiString FindProjectDirectory() const final override;
  virtual xiiString GetProjectDataDirectoryPath() const final override;
  const xiiImage&   GetLastScreenshot() { return m_LastScreenshot; }
  /// Camera must have a global key named "Camera4" if `uiCameraNumber` was 4.
  void SwitchToCamera(xiiUInt32 uiCameraNumber);

  xiiResult LoadScene(xiiStringView sSceneFile);
  xiiWorld* GetWorld() const { return m_pWorld.Borrow(); }

protected:
  virtual xiiResult                      BeforeCoreSystemsStartup() override;
  virtual void                           AfterCoreSystemsStartup() override;
  virtual void                           BeforeHighLevelSystemsShutdown() override;
  virtual void                           StoreScreenshot(xiiImage&& image, xiiStringView sContext) override;
  virtual void                           Init_FileSystem_ConfigureDataDirs() override;
  virtual xiiUniquePtr<xiiGameStateBase> CreateGameState() override;

  xiiString              m_sProjectDirName;
  xiiUniquePtr<xiiWorld> m_pWorld;
  xiiImage               m_LastScreenshot;
};

class xiiGameEngineTest : public xiiTestBaseClass
{
  using SUPER = xiiTestBaseClass;

public:
  xiiGameEngineTest();
  ~xiiGameEngineTest();

  virtual xiiResult                     GetImage(xiiImage& ref_img, const xiiSubTestEntry& subTest, xiiUInt32 uiImageNumber) override;
  virtual xiiGameEngineTestApplication* CreateApplication() = 0;

protected:
  virtual xiiResult InitializeTest() override;
  virtual xiiResult DeInitializeTest() override;
  virtual xiiResult InitializeSubTest(xiiInt32 iIdentifier) override;

  xiiGameEngineTestApplication* m_pApplication = nullptr;
};
