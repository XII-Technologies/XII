#pragma once

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#if XII_ENABLED(XII_PLATFORM_ANDROID)

#  include <Foundation/Basics.h>
#  include <Foundation/Strings/String.h>

class xiiApplication;
struct AInputEvent;

class xiiAndroidApplication
{
public:
  xiiAndroidApplication(struct android_app* pApp, xiiApplication* pXIIApp);
  ~xiiAndroidApplication();

  void    AndroidRun();
  void    HandleCmd(int32_t cmd);
  int32_t HandleInput(AInputEvent* pEvent);
  void    HandleIdent(xiiInt32 iIdent);

private:
  struct android_app* m_pApp;
  xiiApplication*     m_pXIIApp;
};

#endif
