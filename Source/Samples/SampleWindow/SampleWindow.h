#pragma once

#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>

class xiiSampleWindow;

// A simple application that creates a window.
class xiiSampleWindowApp : public xiiApplication
{
public:
  typedef xiiApplication SUPER;

  xiiSampleWindowApp();

  virtual Execution Run() override;

  virtual void AfterCoreSystemsStartup() override;

  virtual void BeforeHighLevelSystemsShutdown() override;

  virtual void BeforeCoreSystemsShutdown() override;

private:
  xiiSampleWindow* m_pWindow = nullptr;
};
