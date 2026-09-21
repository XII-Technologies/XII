/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Basics/Platform/Windows/MinWindows.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

namespace xiiApplicationDetails
{
  XII_FOUNDATION_DLL void      SetConsoleCtrlHandler(xiiMinWindows::BOOL(XII_WINDOWS_WINAPI* consoleHandler)(xiiMinWindows::DWORD dwCtrlType));
  XII_FOUNDATION_DLL xiiMutex& GetShutdownMutex();

  template <typename AppClass, typename... Args>
  int ConsoleEntry(int iArgc, const char** pArgv, Args&&... arguments)
  {
#if XII_ENABLED(XII_COMPILER_MSVC)           // Internal compiler error in MSVC. Can not align buffer otherwise the compiler will crash.
    static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.
#else
    alignas(alignof(AppClass)) static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.
#endif

    // This mutex will prevent the console shutdown handler to return
    // as long as this entry point is not finished executing
    // (see consoleHandler below).
    XII_LOCK(GetShutdownMutex());

    static AppClass* pApp = new (appBuffer) AppClass(std::forward<Args>(arguments)...);
    pApp->SetCommandLineArguments((xiiUInt32)iArgc, pArgv);

    // This handler overrides the default handler
    // (which would call ExitProcess, which leads to disorderly engine shutdowns)
    const auto consoleHandler = [](xiiMinWindows::DWORD ctrlType) -> xiiMinWindows::BOOL {
      // We have to wait until the application has shut down orderly
      // since Windows will kill everything after this handler returns
      pApp->SetReturnCode(ctrlType);
      pApp->RequestQuit();
      XII_LOCK(GetShutdownMutex());
      return 1; // returns TRUE, which deactivates the default console control handler
    };
    SetConsoleCtrlHandler(consoleHandler);

    xiiRun(pApp); // Life cycle & run method calling

    const xiiInt32 iReturnCode = pApp->GetReturnCode();
    if (iReturnCode != 0)
    {
      std::string text = pApp->TranslateReturnCode();
      if (!text.empty())
        xiiLog::Printf("Return Code: %i = '%s'\n", iReturnCode, text.c_str());
      else
        xiiLog::Printf("Return Code: %i\n", iReturnCode, text.c_str());
    }

    const bool memLeaks = pApp->IsMemoryLeakReportingEnabled();
    pApp->~AppClass();
    memset((void*)pApp, 0, sizeof(AppClass));
    if (memLeaks)
    {
      xiiMemoryTracker::DumpMemoryLeaks();
    }
    return iReturnCode;
  }

  template <typename AppClass, typename... Args>
  int ApplicationEntry(Args&&... arguments)
  {
#if XII_ENABLED(XII_COMPILER_MSVC)           // Internal compiler error in MSVC. Can not align buffer otherwise the compiler will crash.
    static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.
#else
    alignas(alignof(AppClass)) static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.
#endif

    AppClass* pApp = new (appBuffer) AppClass(std::forward<Args>(arguments)...);
    pApp->SetCommandLineArguments((xiiUInt32)__argc, const_cast<const char**>(__argv));
    xiiRun(pApp); // Life cycle & run method calling

    const int iReturnCode = pApp->GetReturnCode();
    if (iReturnCode != 0)
    {
      std::string text = pApp->TranslateReturnCode();
      if (!text.empty())
      {
        xiiLog::Printf("Return Code: '%s'\n", text.c_str());
      }
    }

    const bool memLeaks = pApp->IsMemoryLeakReportingEnabled();
    pApp->~AppClass();
    memset((void*)pApp, 0, sizeof(AppClass));
    if (memLeaks)
    {
      xiiMemoryTracker::DumpMemoryLeaks();
    }

    return iReturnCode;
  }
} // namespace xiiApplicationDetails

/// Same as XII_APPLICATION_ENTRY_POINT but should be used for applications that shall always show a console window.
#define XII_CONSOLEAPP_ENTRY_POINT(AppClass, ...)                                                                      \
  /* Enable high performance graphics on laptops with dual graphics cards (e.g. NVIDIA Optimus or AMD PowerXpress). */ \
  extern "C"                                                                                                           \
  {                                                                                                                    \
    _declspec(dllexport) xiiMinWindows::DWORD NvOptimusEnablement                  = 0x00000001;                       \
    _declspec(dllexport) xiiMinWindows::DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;                       \
  }                                                                                                                    \
  XII_APPLICATION_ENTRY_POINT_CODE_INJECTION                                                                           \
  int main(int argc, const char** argv)                                                                                \
  {                                                                                                                    \
    return xiiApplicationDetails::ConsoleEntry<AppClass>(argc, argv, __VA_ARGS__);                                     \
  }

// If windows.h is already included use the native types, otherwise use types from xiiMinWindows
//
// In XII_APPLICATION_ENTRY_POINT we use macro magic to concatenate strings in such a way that depending on whether windows.h has
// been included in the mean time, either the macro is chosen which expands to the proper Windows.h type
// or the macro that expands to our xiiMinWindows type.
// Unfortunately we cannot do the decision right here, as Windows.h may not yet be included, but may get included later.
#define _XII_APPLICATION_ENTRY_POINT_HINSTANCE          HINSTANCE
#define _XII_APPLICATION_ENTRY_POINT_LPSTR              LPSTR
#define _XII_APPLICATION_ENTRY_POINT_HINSTANCE_WINDOWS_ xiiMinWindows::HINSTANCE
#define _XII_APPLICATION_ENTRY_POINT_LPSTR_WINDOWS_     xiiMinWindows::LPSTR

#ifndef _In_
#  define UndefSAL
#  define _In_
#  define _In_opt_
#endif

/// This macro allows for easy creation of application entry points (since they can't be placed in DLLs)
///
/// Just use the macro in a cpp file of your application and supply your app class (must be derived from xiiApplication).
/// The additional (optional) parameters are passed to the constructor of your app class.
#define XII_APPLICATION_ENTRY_POINT(AppClass, ...)                                                                                           \
  /* Enable high performance graphics on laptops with dual graphics cards (e.g. NVIDIA Optimus or AMD PowerXpress). */                       \
  extern "C"                                                                                                                                 \
  {                                                                                                                                          \
    _declspec(dllexport) xiiMinWindows::DWORD NvOptimusEnablement                  = 0x00000001;                                             \
    _declspec(dllexport) xiiMinWindows::DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;                                             \
  }                                                                                                                                          \
  XII_APPLICATION_ENTRY_POINT_CODE_INJECTION                                                                                                 \
  int XII_WINDOWS_CALLBACK WinMain(_In_     XII_PP_CONCAT(_XII_, XII_PP_CONCAT(APPLICATION_ENTRY_POINT_HINSTANCE, _WINDOWS_)) hInstance,     \
                                   _In_opt_ XII_PP_CONCAT(_XII_, XII_PP_CONCAT(APPLICATION_ENTRY_POINT_HINSTANCE, _WINDOWS_)) hPrevInstance, \
                                   _In_     XII_PP_CONCAT(_XII_, XII_PP_CONCAT(APPLICATION_ENTRY_POINT_LPSTR, _WINDOWS_)) lpCmdLine,         \
                                   _In_ int nCmdShow)                                                                                        \
  {                                                                                                                                          \
    return xiiApplicationDetails::ApplicationEntry<AppClass>(__VA_ARGS__);                                                                   \
  }

#ifdef UndefSAL
#  undef _In_
#  undef _In_opt_
#endif
