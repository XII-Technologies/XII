#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_ANDROID)

#  include <Foundation/Application/Application.h>
#  include <Foundation/Application/Implementation/Android/Application_android.h>
#  include <android/log.h>
#  include <android_native_app_glue.h>

static void xiiAndroidHandleCmd(struct android_app* pApp, int32_t cmd)
{
  xiiAndroidApplication* pAndroidApp = static_cast<xiiAndroidApplication*>(pApp->userData);
  pAndroidApp->HandleCmd(cmd);
}

static int32_t xiiAndroidHandleInput(struct android_app* pApp, AInputEvent* pEvent)
{
  xiiAndroidApplication* pAndroidApp = static_cast<xiiAndroidApplication*>(pApp->userData);
  return pAndroidApp->HandleInput(pEvent);
}

xiiAndroidApplication::xiiAndroidApplication(struct android_app* pApp, xiiApplication* pXIIApp) :
  m_pApp(pApp), m_pXIIApp(pXIIApp)
{
  pApp->userData     = this;
  pApp->onAppCmd     = xiiAndroidHandleCmd;
  pApp->onInputEvent = xiiAndroidHandleInput;
  //#TODO: acquire sensors, set app->onAppCmd, set app->onInputEvent
}

xiiAndroidApplication::~xiiAndroidApplication() {}

void xiiAndroidApplication::AndroidRun()
{
  bool bRun = true;
  while (true)
  {
    struct android_poll_source* pSource = nullptr;
    int                         iIdent  = 0;
    int                         iEvents = 0;
    while ((iIdent = ALooper_pollAll(0, nullptr, &iEvents, (void**)&pSource)) >= 0)
    {
      if (pSource != nullptr)
        pSource->process(m_pApp, pSource);

      HandleIdent(iIdent);
    }
    if (bRun && m_pXIIApp->Run() != xiiApplication::Execution::Continue)
    {
      bRun = false;
      ANativeActivity_finish(m_pApp->activity);
    }
    if (m_pApp->destroyRequested)
    {
      break;
    }
  }
}

void xiiAndroidApplication::HandleCmd(int32_t cmd)
{
  //#TODO:
}

int32_t xiiAndroidApplication::HandleInput(AInputEvent* pEvent)
{
  //#TODO:
  return 0;
}

void xiiAndroidApplication::HandleIdent(xiiInt32 iIdent)
{
  //#TODO:
}

XII_FOUNDATION_DLL void xiiAndroidRun(struct android_app* pApp, xiiApplication* pXIIApp)
{
  xiiAndroidApplication androidApp(pApp, pXIIApp);

  if (xiiRun_Startup(pXIIApp).Succeeded())
  {
    androidApp.AndroidRun();
  }
  xiiRun_Shutdown(pXIIApp);

  const int iReturnCode = pXIIApp->GetReturnCode();
  if (iReturnCode != 0)
  {
    const char* szReturnCode = pXIIApp->TranslateReturnCode();
    if (szReturnCode != nullptr && szReturnCode[0] != '\0')
      __android_log_print(ANDROID_LOG_ERROR, "xiiEngine", "Return Code: '%s'", szReturnCode);
  }
}

#endif

XII_STATICLINK_FILE(Foundation, Foundation_Application_Implementation_Android_Application_android);
