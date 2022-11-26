#include <GameEngineTest/GameEngineTestPCH.h>

#include "TestClass.h"
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <RendererFoundation/Device/Device.h>

xiiGameEngineTest::xiiGameEngineTest()  = default;
xiiGameEngineTest::~xiiGameEngineTest() = default;

xiiResult xiiGameEngineTest::GetImage(xiiImage& img)
{
  img.ResetAndCopy(m_pApplication->GetLastScreenshot());

  return XII_SUCCESS;
}

xiiResult xiiGameEngineTest::InitializeTest()
{
  m_pApplication = CreateApplication();

  if (m_pApplication == nullptr)
    return XII_FAILURE;


  XII_SUCCEED_OR_RETURN(xiiRun_Startup(m_pApplication));

  if (xiiStringUtils::IsEqual_NoCase(xiiGameApplication::GetActiveRenderer(), "DX11"))
  {
    if (xiiGALDevice::HasDefaultDevice() && (xiiGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName == "Microsoft Basic Render Driver" || xiiGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.StartsWith_NoCase("Intel(R) UHD Graphics")))
    {
      // Use different images for comparison when running the D3D11 Reference Device
      xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_D3D11Ref");
    }
    else if (xiiStringUtils::IsEqual_NoCase(xiiGameApplication::GetActiveRenderer(), "DX11") && xiiGALDevice::HasDefaultDevice() && xiiGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("AMD") || xiiGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Radeon"))
    {
      // Line rendering on DX11 is different on AMD and requires separate images for tests rendering lines.
      xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_AMD");
    }
    else
    {
      xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("");
    }
  }
  else if (xiiStringUtils::IsEqual_NoCase(xiiGameApplication::GetActiveRenderer(), "Vulkan"))
  {
    if (xiiGALDevice::HasDefaultDevice() && xiiGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("llvmpipe"))
    {
      xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_LLVMPIPE");
    }
    else
    {
      xiiTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_Vulkan");
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiGameEngineTest::DeInitializeTest()
{
  if (m_pApplication)
  {
    m_pApplication->RequestQuit();

    xiiInt32 iSteps = 2;
    while (m_pApplication->Run() == xiiApplication::Execution::Continue && iSteps > 0)
    {
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

  xiiResourceManager::ForceNoFallbackAcquisition(3);

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////


xiiGameEngineTestApplication::xiiGameEngineTestApplication(const char* szProjectDirName) :
  xiiGameApplication("xiiGameEngineTest", nullptr)
{
  m_pWorld          = nullptr;
  m_sProjectDirName = szProjectDirName;
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

xiiResult xiiGameEngineTestApplication::LoadScene(const char* szSceneFile)
{
  XII_LOCK(m_pWorld->GetWriteMarker());
  m_pWorld->Clear();
  m_pWorld->GetRandomNumberGenerator().Initialize(42);      // reset the RNG
  m_pWorld->GetClock().SetAccumulatedTime(xiiTime::Zero()); // reset the world clock

  xiiFileReader file;

  if (file.Open(szSceneFile).Succeeded())
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
    xiiLog::Error("Failed to load scene '{0}'", szSceneFile);
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
  m_pWorld->GetClock().SetFixedTimeStep(xiiTime::Seconds(1.0 / 30.0));

  ActivateGameState(m_pWorld.Borrow()).IgnoreResult();
}

void xiiGameEngineTestApplication::BeforeHighLevelSystemsShutdown()
{
  m_pWorld = nullptr;

  SUPER::BeforeHighLevelSystemsShutdown();
}

void xiiGameEngineTestApplication::StoreScreenshot(xiiImage&& image, const char* szContext)
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

    xiiFileSystem::AddDataDirectory(">xiitest/", "ImageComparisonDataDir", "imgout", xiiFileSystem::AllowWrites).IgnoreResult();
    xiiFileSystem::AddDataDirectory(sReadDir, "ImageComparisonDataDir").IgnoreResult();
  }
}

xiiUniquePtr<xiiGameStateBase> xiiGameEngineTestApplication::CreateGameState(xiiWorld* pWorld)
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

xiiGameStatePriority xiiGameEngineTestGameState::DeterminePriority(xiiWorld* pWorld) const
{
  return xiiGameStatePriority::Default;
}

void xiiGameEngineTestGameState::ConfigureInputActions()
{
  // do nothing
}
