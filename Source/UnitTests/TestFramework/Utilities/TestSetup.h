/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <TestFramework/TestFrameworkDLL.h>

class xiiTestFramework;

/// A collection of static helper functions to setup the test framework.
class XII_TEST_DLL xiiTestSetup
{
public:
  /// Creates and returns a test framework with the given name.
  static xiiTestFramework* InitTestFramework(const char* szTestName, const char* szNiceTestName, int iArgc, const char** pArgv);

  /// Runs tests and returns number of errors.
  static xiiTestAppRun RunTests();

  static xiiInt32 GetFailedTestCount();

  /// Deletes the test framework and outputs final test output.
  ///
  /// If bSilent is true, the function will not print anything to the console (debug info)
  static void DeInitTestFramework(bool bSilent = false);

private:
  static int          s_iArgc;
  static const char** s_pArgv;
};
