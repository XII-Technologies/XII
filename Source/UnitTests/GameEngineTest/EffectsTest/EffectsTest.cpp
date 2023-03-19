#include <GameEngineTest/GameEngineTestPCH.h>

#include "EffectsTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Components/ParticleComponent.h>

static xiiGameEngineTestEffects s_GameEngineTestEffects;

const char* xiiGameEngineTestEffects::GetTestName() const
{
  return "Effects Tests";
}

xiiGameEngineTestApplication* xiiGameEngineTestEffects::CreateApplication()
{
  m_pOwnApplication = XII_DEFAULT_NEW(xiiGameEngineTestApplication, "Effects");
  return m_pOwnApplication;
}

void xiiGameEngineTestEffects::SetupSubTests()
{
  AddSubTest("Decals", SubTests::Decals);
  AddSubTest("Heightfield", SubTests::Heightfield);
  AddSubTest("WindClothRopes", SubTests::WindClothRopes);
  AddSubTest("Reflections", SubTests::Reflections);
  AddSubTest("StressTest", SubTests::StressTest);
}

xiiResult xiiGameEngineTestEffects::InitializeSubTest(xiiInt32 iIdentifier)
{
  XII_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame       = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Decals)
  {
    m_ImgCompFrames.PushBack({5});
    m_ImgCompFrames.PushBack({30});
    m_ImgCompFrames.PushBack({60});

    XII_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Decals.xiiObjectGraph"));
    return XII_SUCCESS;
  }

  if (iIdentifier == SubTests::Heightfield)
  {
    m_ImgCompFrames.PushBack({20});

    XII_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Heightfield.xiiObjectGraph"));
    return XII_SUCCESS;
  }

  if (iIdentifier == SubTests::WindClothRopes)
  {
    m_ImgCompFrames.PushBack({20, 550});
    m_ImgCompFrames.PushBack({100, 600});

    XII_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Wind.xiiObjectGraph"));
    return XII_SUCCESS;
  }
  if (iIdentifier == SubTests::Reflections)
  {
    m_ImgCompFrames.PushBack({30});

    XII_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/Reflections.xiiObjectGraph"));
    return XII_SUCCESS;
  }
  if (iIdentifier == SubTests::StressTest)
  {
    m_ImgCompFrames.PushBack({100});

    XII_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Effects/AssetCache/Common/Scenes/StressTest.xiiObjectGraph"));
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiTestAppRun xiiGameEngineTestEffects::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  ++m_iFrame;

  if (m_pOwnApplication->Run() == xiiApplication::Execution::Quit)
    return xiiTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx].m_uiFrame == m_iFrame)
  {
    XII_TEST_IMAGE(m_uiImgCompIdx, m_ImgCompFrames[m_uiImgCompIdx].m_uiThreshold);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      if (false)
      {
        xiiProfilingSystem::ProfilingData profilingData;
        xiiProfilingSystem::Capture(profilingData);

        xiiStringBuilder sPath(":appdata/Profiling/", xiiApplication::GetApplicationInstance()->GetApplicationName());
        sPath.AppendPath("effectsProfiling.json");

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
