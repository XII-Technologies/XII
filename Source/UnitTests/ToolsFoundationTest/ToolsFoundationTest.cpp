/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

xiiInt32 xiiConstructionCounter::s_iConstructions     = 0;
xiiInt32 xiiConstructionCounter::s_iDestructions      = 0;
xiiInt32 xiiConstructionCounter::s_iConstructionsLast = 0;
xiiInt32 xiiConstructionCounter::s_iDestructionsLast  = 0;

XII_TESTFRAMEWORK_ENTRY_POINT("ToolsFoundationTest", "Tools Foundation Tests")
