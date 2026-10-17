/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Configuration/SubSystem.h>
#include <FoundationTest/FoundationTestPCH.h>

// Keep both endings on the same physical line: unity builds must not identify
// subsystem instances solely by their source line in the original cpp file.
// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(FoundationTest, SameLineFirst) XII_END_SUBSYSTEM_DECLARATION; XII_BEGIN_SUBSYSTEM_DECLARATION(FoundationTest, SameLineSecond) XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_CREATE_SIMPLE_TEST(Configuration, SubSystemDeclarationIdentity)
{
  xiiUInt32 first = 0, second = 0;
  for (auto* subsystem = xiiSubSystem::GetFirstInstance(); subsystem; subsystem = subsystem->GetNextInstance())
  {
    if (subsystem->GetGroupName() != "FoundationTest")
      continue;
    first += subsystem->GetSubSystemName() == "SameLineFirst" ? 1U : 0U;
    second += subsystem->GetSubSystemName() == "SameLineSecond" ? 1U : 0U;
  }
  XII_TEST_INT(first, 1);
  XII_TEST_INT(second, 1);
}
