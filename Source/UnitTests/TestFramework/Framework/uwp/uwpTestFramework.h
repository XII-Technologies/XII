#pragma once

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)

#  include <TestFramework/Framework/TestFramework.h>
#  include <TestFramework/TestFrameworkDLL.h>

#  include <Foundation/Basics/Platform/uwp/UWPUtils.h>

/// \brief Derived xiiTestFramework which signals the GUI to update whenever a new tests result comes in.
class XII_TEST_DLL xiiUwpTestFramework : public xiiTestFramework
{
public:
  xiiUwpTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int argc, const char** argv);
  virtual ~xiiUwpTestFramework();

  xiiUwpTestFramework(xiiUwpTestFramework&) = delete;
  void operator=(xiiUwpTestFramework&) = delete;

  void Run();
};

#endif
