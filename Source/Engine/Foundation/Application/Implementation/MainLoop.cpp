/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>

xiiResult xiiRun_Startup(xiiApplication* pApplicationInstance)
{
  XII_ASSERT_ALWAYS(pApplicationInstance != nullptr, "xiiRun() requires a valid non-null application instance pointer.");
  XII_ASSERT_ALWAYS(xiiApplication::s_pApplicationInstance == nullptr, "There can only be one xiiApplication.");

  // Set application instance pointer to the supplied instance.
  xiiApplication::s_pApplicationInstance = pApplicationInstance;

  XII_SUCCEED_OR_RETURN(pApplicationInstance->BeforeCoreSystemsStartup());

  // This will startup all Base and Core systems.
  // 'StartupHighLevelSystems' must not be done before a window is available (if at all) so we don't do that here.
  xiiStartup::StartupCoreSystems();

  pApplicationInstance->AfterCoreSystemsStartup();

  return XII_SUCCESS;
}

void xiiRun_MainLoop(xiiApplication* pApplicationInstance)
{
  while (pApplicationInstance->Run() == xiiApplication::Execution::Continue)
  {
  }
}

void xiiRun_Shutdown(xiiApplication* pApplicationInstance)
{
  // High Level Systems Shutdown.
  // This may do nothing, if the high level systems were never initialized.
  {
    pApplicationInstance->BeforeHighLevelSystemsShutdown();
    xiiStartup::ShutdownHighLevelSystems();
    pApplicationInstance->AfterHighLevelSystemsShutdown();
  }

  // Core Systems Shutdown.
  {
    pApplicationInstance->BeforeCoreSystemsShutdown();
    xiiStartup::ShutdownCoreSystems();
    pApplicationInstance->AfterCoreSystemsShutdown();
  }

  // Flush standard output to make log available.
  fflush(stdout);
  fflush(stderr);

  // Reset application instance so code running after the app will trigger asserts etc. to be cleaned up
  // Destructor is called by entry point function
  xiiApplication::s_pApplicationInstance = nullptr;

  // Memory leak reporting cannot be done here, because the application instance is still alive and may still hold on to memory that needs
  // to be freed first.
}

void xiiRun(xiiApplication* pApplicationInstance)
{
  if (xiiRun_Startup(pApplicationInstance).Succeeded())
  {
    xiiRun_MainLoop(pApplicationInstance);
  }
  xiiRun_Shutdown(pApplicationInstance);
}

XII_STATICLINK_FILE(Foundation, Foundation_Application_Implementation_MainLoop);
