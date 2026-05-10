/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngineTest/GameEngineTestPCH.h>

#include "StateMachineTest.h"
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>

static xiiGameEngineTestStateMachine s_GameEngineTestAnimations;

const char* xiiGameEngineTestStateMachine::GetTestName() const
{
  return "StateMachine Tests";
}

xiiGameEngineTestApplication* xiiGameEngineTestStateMachine::CreateApplication()
{
  m_pOwnApplication = XII_DEFAULT_NEW(xiiGameEngineTestApplication, "StateMachine");
  return m_pOwnApplication;
}

void xiiGameEngineTestStateMachine::SetupSubTests()
{
  AddSubTest("Builtins", SubTests::Builtins);
}

xiiResult xiiGameEngineTestStateMachine::InitializeSubTest(xiiInt32 iIdentifier)
{
  XII_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame       = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::Builtins)
  {
    return XII_SUCCESS;
  }
  return XII_FAILURE;
}

xiiTestAppRun xiiGameEngineTestStateMachine::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  if (iIdentifier == SubTests::Builtins)
  {
    RunBuiltinsTest();
    return xiiTestAppRun::Quit;
  }

  const bool bVulkan = xiiGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan");
  ++m_iFrame;

  m_pOwnApplication->Run();
  if (m_pOwnApplication->WasQuitRequested())
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
