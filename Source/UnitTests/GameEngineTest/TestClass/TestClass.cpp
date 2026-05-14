/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngineTest/GameEngineTestPCH.h>

#include "TestClass.h"
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsFoundation/Device/Device.h>

xiiGameEngineTest::xiiGameEngineTest()  = default;
xiiGameEngineTest::~xiiGameEngineTest() = default;

xiiResult xiiGameEngineTest::GetImage(xiiImage& ref_img, const xiiSubTestEntry& subTest, xiiUInt32 uiImageNumber)
{
  ref_img.ResetAndCopy(m_pApplication->GetLastScreenshot());

  return XII_SUCCESS;
}

xiiResult xiiGameEngineTest::InitializeTest()
{
  m_pApplication = CreateApplication();

  if (m_pApplication == nullptr)
    return XII_FAILURE;

  XII_SUCCEED_OR_RETURN(xiiRun_Startup(m_pApplication));

  return XII_SUCCESS;
}

xiiResult xiiGameEngineTest::DeInitializeTest()
{
  if (m_pApplication)
  {
    m_pApplication->RequestQuit();

    xiiInt32 iSteps = 2;
    while (!m_pApplication->WasQuitRequested() && iSteps > 0)
    {
      m_pApplication->Run();
      --iSteps;
    }

    xiiRun_Shutdown(m_pApplication);

    XII_DEFAULT_DELETE(m_pApplication);

    if (iSteps == 0)
      return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiGameEngineTest::InitializeSubTest(xiiInt32 iIdentifier)
{
  XII_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////


xiiGameEngineTestApplication::xiiGameEngineTestApplication(xiiStringView sProjectDirName) :
  xiiGameApplication("xiiGameEngineTest", nullptr)
{
  m_pWorld          = nullptr;
  m_sProjectDirName = sProjectDirName;
}

xiiString xiiGameEngineTestApplication::FindProjectDirectory() const
{
  return m_sAppProjectPath;
}

xiiString xiiGameEngineTestApplication::GetProjectDataDirectoryPath() const
{
  xiiStringBuilder sProjectPath(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath(), "/", m_sProjectDirName);
  return sProjectPath;
}

void xiiGameEngineTestApplication::SwitchToCamera(xiiUInt32 uiCameraNumber)
{
  xiiWorld* pWorld = GetWorld();
  XII_LOCK(pWorld->GetReadMarker());
  xiiGameObject*   pCamera = nullptr;
  xiiStringBuilder sCamera;
  sCamera.SetFormat("Camera{}", uiCameraNumber);
  if (pWorld->TryGetObjectWithGlobalKey(xiiTempHashedString(sCamera), pCamera))
  {
    xiiCameraComponent* pCameraComponent = nullptr;
    if (pCamera->TryGetComponentOfBaseType<xiiCameraComponent>(pCameraComponent))
    {
      // update view camera
      xiiCamera*           pCamera = xiiDynamicCast<xiiGameState*>(GetActiveGameState())->GetMainCamera();
      const xiiGameObject* pOwner  = pCameraComponent->GetOwner();
      const xiiVec3        vPos    = pOwner->GetGlobalPosition();
      const xiiVec3        vFwd    = pOwner->GetGlobalDirForwards();
      const xiiVec3        vUp     = pOwner->GetGlobalDirUp();
      pCamera->LookAt(vPos, vPos + vFwd, vUp);
      pCamera->SetCameraMode(pCameraComponent->GetCameraMode(), pCameraComponent->GetFieldOfView(), pCameraComponent->GetNearPlane(), pCameraComponent->GetFarPlane());
    }
  }
}

xiiResult xiiGameEngineTestApplication::LoadScene(xiiStringView sSceneFile)
{
  XII_LOCK(m_pWorld->GetWriteMarker());
  m_pWorld->Clear();
  m_pWorld->GetRandomNumberGenerator().Initialize(42); // reset the RNG
  m_pWorld->GetClock().Reset(false);                   // reset the world clock

  xiiFileReader file;

  if (file.Open(sSceneFile).Succeeded())
  {
    // File Header
    {
      xiiAssetFileHeader header;
      XII_SUCCEED_OR_RETURN(header.Read(file));

      char szSceneTag[16];
      file.ReadBytes(szSceneTag, sizeof(char) * 16);

      XII_ASSERT_RELEASE(xiiStringUtils::IsEqualN(szSceneTag, "[xiiBinaryScene]", 16), "The given file is not a valid scene file");
    }

    xiiWorldReader reader;
    XII_SUCCEED_OR_RETURN(reader.ReadWorldDescription(file));
    reader.InstantiateWorld(*m_pWorld, nullptr);

    return XII_SUCCESS;
  }
  else
  {
    xiiLog::Error("Failed to load scene '{0}'", sSceneFile);
    return XII_FAILURE;
  }
}

xiiResult xiiGameEngineTestApplication::BeforeCoreSystemsStartup()
{
  XII_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  xiiStringBuilder sProject;
  XII_SUCCEED_OR_RETURN(xiiFileSystem::ResolveSpecialDirectory(GetProjectDataDirectoryPath(), sProject));
  m_sAppProjectPath = sProject;

  return XII_SUCCESS;
}

void xiiGameEngineTestApplication::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  xiiStartup::StartupHighLevelSystems();

  xiiWorldDesc desc("GameEngineTestWorld");
  desc.m_uiRandomNumberGeneratorSeed = 42;

  m_pWorld = XII_DEFAULT_NEW(xiiWorld, desc);
  m_pWorld->GetClock().SetFixedTimeStep(xiiTime::MakeFromSeconds(1.0 / 30.0));
  // Disable VSync. Tests run at a fixed time step so this makes tests much faster without changing the outcome.
  xiiGameApplication::cvar_AppVSync = false;

  ActivateGameState(m_pWorld.Borrow(), {}, xiiTransform::MakeIdentity());
}

void xiiGameEngineTestApplication::BeforeHighLevelSystemsShutdown()
{
  SUPER::BeforeHighLevelSystemsShutdown();

  m_pWorld = nullptr;
}

void xiiGameEngineTestApplication::StoreScreenshot(xiiImage&& image, xiiStringView sContext)
{
  // store this for image comparison purposes
  m_LastScreenshot.ResetAndMove(std::move(image));
}

void xiiGameEngineTestApplication::Init_FileSystem_ConfigureDataDirs()
{
  SUPER::Init_FileSystem_ConfigureDataDirs();

  // additional data directories for the tests to work
  {
    xiiFileSystem::SetSpecialDirectory("testout", xiiTestFramework::GetInstance()->GetAbsOutputPath());

    xiiStringBuilder sBaseDir = ">sdk/Data/Base/";
    xiiStringBuilder sReadDir(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());

    xiiFileSystem::AddDataDirectory(">xiitest/", "ImageComparisonDataDir", "imgout", xiiDataDirUsage::AllowWrites).IgnoreResult();
    xiiFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir").IgnoreResult();
  }
}

xiiUniquePtr<xiiGameStateBase> xiiGameEngineTestApplication::CreateGameState()
{
  return XII_DEFAULT_NEW(xiiGameEngineTestGameState);
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGameEngineTestGameState, 1, xiiRTTIDefaultAllocator<xiiGameEngineTestGameState>)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiGameEngineTestGameState::ProcessInput()
{
  // Do nothing, user input should be ignored

  // trigger taking a screenshot every frame, for image comparison purposes
  xiiGameApplicationBase::GetGameApplicationBaseInstance()->TakeScreenshot();
}

void xiiGameEngineTestGameState::ConfigureInputActions()
{
  // do nothing
}
