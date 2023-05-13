#pragma once

/// \file
#include <Foundation/Application/Application.h>
#include <Foundation/Basics/Platform/Android/AndroidUtils.h>

class xiiApplication;

extern XII_FOUNDATION_DLL void xiiAndroidRun(struct android_app* pAndroidApp, xiiApplication* pApp);

namespace xiiApplicationDetails
{
  template <typename AppClass, typename... Args>
  void EntryFunc(struct android_app* pAndroidApp, Args&&... arguments)
  {
    alignas(XII_ALIGNMENT_OF(AppClass)) static char appBuffer[sizeof(AppClass)]; // Not on the stack to cope with smaller stacks.
    xiiAndroidUtils::SetNativeAndroidApp(pAndroidApp);
    AppClass* pApp = new (appBuffer) AppClass(std::forward<Args>(arguments)...);

    xiiAndroidRun(pAndroidApp, pApp);

    pApp->~AppClass();
    memset(pApp, 0, sizeof(AppClass));
  }
} // namespace xiiApplicationDetails


/// \brief Same as XII_APPLICATION_ENTRY_POINT but should be used for applications that shall always show a console window.
#define XII_CONSOLEAPP_ENTRY_POINT XII_APPLICATION_ENTRY_POINT

/// \brief This macro allows for easy creation of application entry points (since they can't be placed in DLLs)
///
/// Just use the macro in a cpp file of your application and supply your app class (must be derived from xiiApplication).
/// The additional (optional) parameters are passed to the constructor of your app class.
#define XII_APPLICATION_ENTRY_POINT(AppClass, ...)                                                                                \
  alignas(XII_ALIGNMENT_OF(AppClass)) static char appBuffer[sizeof(AppClass)]; /* Not on the stack to cope with smaller stacks */ \
  XII_APPLICATION_ENTRY_POINT_CODE_INJECTION                                                                                      \
  extern "C" void android_main(struct android_app* app) { ::xiiApplicationDetails::EntryFunc<AppClass>(app, __VA_ARGS__); }
