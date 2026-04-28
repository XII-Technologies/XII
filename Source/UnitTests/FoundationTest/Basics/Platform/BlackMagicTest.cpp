/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#define INT_DECLARE(name, n) int name = n;

namespace
{
  XII_EXPAND_ARGS_WITH_INDEX(INT_DECLARE, heinz, klaus);
}

XII_CREATE_SIMPLE_TEST(Basics, BlackMagic)
{
  XII_TEST_INT(heinz, 0);
  XII_TEST_INT(klaus, 1);
}
