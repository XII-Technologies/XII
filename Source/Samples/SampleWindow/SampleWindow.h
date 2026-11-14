#pragma once

#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>

class xiiSampleWindow;

// A simple application that creates a window.
class xiiSampleWindowApp : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  xiiSampleWindowApp();

  Execution Run() override;

  void AfterCoreSystemsStartup() override;

  void BeforeHighLevelSystemsShutdown() override;

  void BeforeCoreSystemsShutdown() override;

private:
  xiiSampleWindow* m_pWindow = nullptr;
};
