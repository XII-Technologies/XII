#include <GameEngineTest/GameEngineTestPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <Core/World/World.h>
#  include <Core/WorldSerializer/WorldReader.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileWriter.h>
#  include <Foundation/Profiling/Profiling.h>
#  include <GameEngineTest/StereoTest/StereoTest.h>
#  include <RendererCore/Components/CameraComponent.h>
#  include <RendererCore/Pipeline/View.h>
#  include <RendererCore/RenderWorld/RenderWorld.h>
#  include <RendererFoundation/Device/Device.h>

static xiiStereoTest s_StereoTest;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStereoTestGameState, 1, xiiRTTIDefaultAllocator<xiiStereoTestGameState>)
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

void xiiStereoTestGameState::OverrideRenderPipeline(xiiTypedResourceHandle<xiiRenderPipelineResource> hPipeline)
{
  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_hMainView, pView))
  {
    pView->SetRenderPipelineResource(hPipeline);
  }
}

//////////////////////////////////////////////////////////////////////////

xiiStereoTestApplication::xiiStereoTestApplication(const char* szProjectDirName) :
  xiiGameEngineTestApplication(szProjectDirName)
{
}

xiiUniquePtr<xiiGameStateBase> xiiStereoTestApplication::CreateGameState(xiiWorld* pWorld)
{
  return XII_DEFAULT_NEW(xiiStereoTestGameState);
}

//////////////////////////////////////////////////////////////////////////

const char* xiiStereoTest::GetTestName() const
{
  return "Stereo Test";
}

xiiGameEngineTestApplication* xiiStereoTest::CreateApplication()
{
  m_pOwnApplication = XII_DEFAULT_NEW(xiiStereoTestApplication, "XR");
  return m_pOwnApplication;
}

void xiiStereoTest::SetupSubTests()
{
  AddSubTest("HoloLensPipeline", SubTests::HoloLensPipeline);
  AddSubTest("DefaultPipeline", SubTests::DefaultPipeline);
}

xiiResult xiiStereoTest::InitializeSubTest(xiiInt32 iIdentifier)
{
  XII_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame       = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::HoloLensPipeline)
  {
    m_ImgCompFrames.PushBack(100);

    XII_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("XR/AssetCache/Common/Scenes/XR.xiiObjectGraph"));

    auto renderPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }");
    xiiDynamicCast<xiiStereoTestGameState*>(m_pOwnApplication->GetActiveGameState())->OverrideRenderPipeline(renderPipeline);

    return XII_SUCCESS;
  }
  if (iIdentifier == SubTests::DefaultPipeline)
  {
    m_ImgCompFrames.PushBack(100);

    XII_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("XR/AssetCache/Common/Scenes/XR.xiiObjectGraph"));

    auto renderPipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }");
    xiiDynamicCast<xiiStereoTestGameState*>(m_pOwnApplication->GetActiveGameState())->OverrideRenderPipeline(renderPipeline);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiTestAppRun xiiStereoTest::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  ++m_iFrame;

  if (m_pOwnApplication->Run() == xiiApplication::Execution::Quit)
    return xiiTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    // The particle effect increases the error on lavapipe, see xiiGameEngineTestParticles::GetImageCompareThreshold.
    xiiUInt32 uiThreshhold = xiiGALDevice::GetDefaultDevice()->GetCapabilities().m_sAdapterName.FindSubString_NoCase("llvmpipe") ? 300 : 250;
    XII_TEST_IMAGE(m_uiImgCompIdx, uiThreshhold);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      if (false)
      {
        xiiProfilingSystem::ProfilingData profilingData;
        xiiProfilingSystem::Capture(profilingData);

        xiiStringBuilder sPath(":appdata/Profiling/", xiiApplication::GetApplicationInstance()->GetApplicationName());
        sPath.AppendPath("stereoProfiling.json");

        xiiFileWriter fileWriter;
        if (fileWriter.Open(sPath) == XII_SUCCESS)
        {
          profilingData.Write(fileWriter).IgnoreResult();
          xiiLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
        }
        else
        {
          xiiLog::Error("Could not write profiling capture to '{0}'.", sPath);
        }
      }

      return xiiTestAppRun::Quit;
    }
  }

  return xiiTestAppRun::Continue;
}

#endif
