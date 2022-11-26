#include <GameEngineTest/GameEngineTestPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include "KrautTest.h"
#  include <Core/WorldSerializer/WorldReader.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <ParticlePlugin/Components/ParticleComponent.h>

static xiiGameEngineTestKraut s_GameEngineTestAnimations;

const char* xiiGameEngineTestKraut::GetTestName() const
{
  return "Kraut Tests";
}

xiiGameEngineTestApplication* xiiGameEngineTestKraut::CreateApplication()
{
  m_pOwnApplication = XII_DEFAULT_NEW(xiiGameEngineTestApplication, "PlatformWin");
  return m_pOwnApplication;
}

void xiiGameEngineTestKraut::SetupSubTests()
{
  AddSubTest("TreeRendering", SubTests::TreeRendering);
}

xiiResult xiiGameEngineTestKraut::InitializeSubTest(xiiInt32 iIdentifier)
{
  XII_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame       = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::TreeRendering)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(60);

    XII_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("PlatformWin/AssetCache/Common/Kraut/Kraut.xiiObjectGraph"));
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiTestAppRun xiiGameEngineTestKraut::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  ++m_iFrame;

  if (m_pOwnApplication->Run() == xiiApplication::Execution::Quit)
    return xiiTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    XII_TEST_IMAGE(m_uiImgCompIdx, 200);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return xiiTestAppRun::Quit;
    }
  }

  return xiiTestAppRun::Continue;
}

#endif
