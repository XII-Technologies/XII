/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Basics.h>

#include <Foundation/Application/Implementation/ApplicationEntryPoint.h>
#include <Foundation/Utilities/CommandLineUtils.h>

class xiiApplication;

/// Platform independent run function for main loop based systems (e.g. Win32, ..)
///
/// This is automatically called by XII_APPLICATION_ENTRY_POINT() and XII_CONSOLEAPP_ENTRY_POINT().
///
/// xiiRun simply calls xiiRun_Startup(), xiiRun_MainLoop() and xiiRun_Shutdown().
XII_FOUNDATION_DLL void xiiRun(xiiApplication* pApplicationInstance);

/// [internal] Called by xiiRun()
XII_FOUNDATION_DLL xiiResult xiiRun_Startup(xiiApplication* pApplicationInstance);
/// [internal] Called by xiiRun()
XII_FOUNDATION_DLL void xiiRun_MainLoop(xiiApplication* pApplicationInstance);
/// [internal] Called by xiiRun()
XII_FOUNDATION_DLL void xiiRun_Shutdown(xiiApplication* pApplicationInstance);

/// Base class to be used by applications based on XII.
///
/// The platform abstraction layer will ensure that the correct functions are called independent of the basic main loop structure
/// (traditional or event-based). Derive an application specific class from xiiApplication and implement at least the abstract Run()
/// function. Additional virtual functions allow to hook into specific events to run application specific code at the correct times.
///
/// Finally pass the name of your derived class to one of the macros XII_APPLICATION_ENTRY_POINT() or XII_CONSOLEAPP_ENTRY_POINT().
/// Those are used to abstract away the platform specific code to run an application.
///
/// A simple example how to get started is as follows:
///
/// \code{.cpp}
///   class xiiSampleApp : public xiiApplication
///   {
///   public:
///
///     virtual void AfterCoreSystemsStartup() override
///     {
///       // Setup Filesystem, Logging, etc.
///     }
///
///     virtual void BeforeCoreSystemsShutdown() override
///     {
///       // Close log file, etc.
///     }
///
///     virtual xiiApplication::Execution Run() override
///     {
///       // Either run a one-time application (e.g. console script) and return xiiApplication::Quit
///       // Or run one update (frame) of your game loop and return xiiApplication::Continue
///
///       return xiiApplication::Quit;
///     }
///   };
///
///   XII_APPLICATION_ENTRY_POINT(xiiSampleApp);
/// \endcode
class XII_FOUNDATION_DLL xiiApplication
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiApplication);

public:
  /// Defines the possible return values for the xiiApplication::Run() function.
  enum class Execution : xiiUInt8
  {
    Continue = 0U, ///< The 'Run' function should return this to keep the application running
    Quit,          ///< The 'Run' function should return this to quit the application
  };

  /// Constructor.
  xiiApplication(xiiStringView sAppName);

  /// Virtual destructor.
  virtual ~xiiApplication();

  /// Changes the application name
  void SetApplicationName(xiiStringView sAppName);

  /// Returns the application name
  const xiiString& GetApplicationName() const { return m_sAppName; }

  /// This function is called before any kind of engine initialization is done.
  ///
  /// Override this function to be able to configure subsystems, before they are initialized.
  /// After this function returns, xiiStartup::StartupCoreSystems() is automatically called.
  /// If you need to set up custom allocators, this is the place to do this.
  virtual xiiResult BeforeCoreSystemsStartup();

  /// This function is called after basic engine initialization has been done.
  ///
  /// xiiApplication will automatically call xiiStartup::StartupCoreSystems() to initialize the application.
  /// This function can be overridden to do additional application specific initialization.
  /// To startup entire subsystems, you should however use the features provided by xiiStartup and xiiSubSystem.
  virtual void AfterCoreSystemsStartup() {}

  /// This function is called after the application main loop has run for the last time, before engine deinitialization.
  ///
  /// After this function call, xiiApplication executes xiiStartup::ShutdownHighLevelSystems().
  ///
  /// \note xiiApplication does NOT call xiiStartup::StartupHighLevelSystems() as it may be a window-less application.
  /// This is left to xiiGameApplicationBase to do. However, it does make sure to shut down the high-level systems,
  /// in case they were started.
  virtual void BeforeHighLevelSystemsShutdown() {}

  /// Called after xiiStartup::ShutdownHighLevelSystems() has been executed.
  virtual void AfterHighLevelSystemsShutdown() {}

  /// This function is called after the application main loop has run for the last time, before engine deinitialization.
  ///
  /// Override this function to do application specific deinitialization that still requires a running engine.
  /// After this function returns xiiStartup::ShutdownCoreSystems() is called and thus everything, including allocators, is shut down.
  /// To shut down entire subsystems, you should, however, use the features provided by xiiStartup and xiiSubSystem.
  virtual void BeforeCoreSystemsShutdown() {}

  /// This function is called after xiiStartup::ShutdownCoreSystems() has been called.
  ///
  /// It is unlikely that there is any kind of deinitialization left, that can still be run at this point.
  virtual void AfterCoreSystemsShutdown() {}

  /// This function is called when an application is moved to the background.
  ///
  /// On Windows that might simply mean that the main window lost the focus.
  /// On other devices this might mean that the application is not visible at all anymore and
  /// might even get shut down later. Override this function to be able to put the application
  /// into a proper sleep mode.
  virtual void BeforeEnterBackground() {}

  /// This function is called whenever an application is resumed from background mode.
  ///
  /// On Windows that might simply mean that the main window received focus again.
  /// On other devices this might mean that the application was suspended and is now active again.
  /// Override this function to reload the apps state or other resources, etc.
  virtual void BeforeEnterForeground() {}

  /// Main run function which is called periodically. This function must be overridden.
  ///
  /// Return Execution::Quit when the application should quit. You may set a return code via SetReturnCode() beforehand.
  virtual Execution Run() = 0;

  /// Sets the value that the application will return to the OS.
  /// You can call this function at any point during execution to update the return value of the application.
  /// Default is zero.
  inline void SetReturnCode(xiiInt32 iReturnCode) { m_iReturnCode = iReturnCode; }

  /// Returns the currently set value that the application will return to the OS.
  inline xiiInt32 GetReturnCode() const { return m_iReturnCode; }

  /// If the return code is not zero, this function might be called to get a string to print the error code in human readable form.
  virtual const char* TranslateReturnCode() const { return ""; }

  /// Will set the command line arguments that were passed to the app by the OS.
  /// This is automatically called by XII_APPLICATION_ENTRY_POINT() and XII_CONSOLEAPP_ENTRY_POINT().
  void SetCommandLineArguments(xiiUInt32 uiArgumentCount, const char** pArguments);

  /// Returns the one instance of xiiApplication that is available.
  static xiiApplication* GetApplicationInstance() { return s_pApplicationInstance; }

  /// Returns the number of command line arguments that were passed to the application.
  ///
  /// Note that the very first command line argument is typically the path to the application itself.
  xiiUInt32 GetArgumentCount() const { return m_uiArgumentCount; }

  /// Returns one of the command line arguments that was passed to the application.
  const char* GetArgument(xiiUInt32 uiArgument) const;

  /// Returns the complete array of command line arguments that were passed to the application.
  const char** GetArgumentsArray() const { return m_pArguments; }

  void EnableMemoryLeakReporting(bool bEnable) { m_bReportMemoryLeaks = bEnable; }

  bool IsMemoryLeakReportingEnabled() const { return m_bReportMemoryLeaks; }

  /// Calling this function requests that the application quits after the current invocation of Run() finishes.
  ///
  /// Sets the m_bWasQuitRequested to true as an indicator for derived application objects to engage shutdown procedures.
  /// Can be overridden to implement custom behavior. There is no other logic associated with this
  /// function and flag so the respective derived application class has to implement logic to perform the actual
  /// quit when this function is called or m_bWasQuitRequested is set to true.
  virtual void RequestQuit();

  /// Returns whether RequestQuit() was called.
  XII_ALWAYS_INLINE bool WasQuitRequested() const { return m_bWasQuitRequested; }

protected:
  bool m_bWasQuitRequested = false;

private:
  xiiInt32 m_iReturnCode = 0;

  xiiUInt32 m_uiArgumentCount = 0;

  const char** m_pArguments = nullptr;

  bool m_bReportMemoryLeaks = true;

  xiiString m_sAppName;

  static xiiApplication* s_pApplicationInstance;

  friend XII_FOUNDATION_DLL_FRIEND void      xiiRun(xiiApplication* pApplicationInstance);
  friend XII_FOUNDATION_DLL_FRIEND xiiResult xiiRun_Startup(xiiApplication* pApplicationInstance);
  friend XII_FOUNDATION_DLL_FRIEND void      xiiRun_MainLoop(xiiApplication* pApplicationInstance);
  friend XII_FOUNDATION_DLL_FRIEND void      xiiRun_Shutdown(xiiApplication* pApplicationInstance);
};
