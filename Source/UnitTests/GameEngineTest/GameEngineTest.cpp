/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngineTest/GameEngineTestPCH.h>

#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

XII_TESTFRAMEWORK_ENTRY_POINT_BEGIN("GameEngineTest", "GameEngine Tests")
{
  xiiTestFramework::GetInstance()->SetTestTimeout(1000 * 60 * 20);
  xiiTestFramework::s_bCallstackOnAssert = true;
}
XII_TESTFRAMEWORK_ENTRY_POINT_END()
