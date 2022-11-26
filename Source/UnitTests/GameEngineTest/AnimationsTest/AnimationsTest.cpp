#include <GameEngineTest/GameEngineTestPCH.h>

#include "AnimationsTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

static xiiGameEngineTestAnimations s_GameEngineTestAnimations;

const char* xiiGameEngineTestAnimations::GetTestName() const
{
  return "Animations Tests";
}

xiiGameEngineTestApplication* xiiGameEngineTestAnimations::CreateApplication()
{
  m_pOwnApplication = XII_DEFAULT_NEW(xiiGameEngineTestApplication, "Animations");
  return m_pOwnApplication;
}

void xiiGameEngineTestAnimations::SetupSubTests()
{
  AddSubTest("Skeletal", SubTests::Skeletal);
}

xiiResult xiiGameEngineTestAnimations::InitializeSubTest(xiiInt32 iIdentifier)
{
  XII_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame       = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Skeletal)
  {
    m_ImgCompFrames.PushBack(1);
    m_ImgCompFrames.PushBack(30);
    m_ImgCompFrames.PushBack(60);

    XII_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("Animations/AssetCache/Common/Scenes/AnimController.xiiObjectGraph"));
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiTestAppRun xiiGameEngineTestAnimations::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  const bool bVulkan = xiiStringUtils::IsEqual_NoCase(xiiGameApplication::GetActiveRenderer(), "Vulkan");
  ++m_iFrame;

  if (m_pOwnApplication->Run() == xiiApplication::Execution::Quit)
    return xiiTestAppRun::Quit;

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    XII_TEST_IMAGE(m_uiImgCompIdx, bVulkan ? 300 : 250);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return xiiTestAppRun::Quit;
    }
  }

  return xiiTestAppRun::Continue;
}
