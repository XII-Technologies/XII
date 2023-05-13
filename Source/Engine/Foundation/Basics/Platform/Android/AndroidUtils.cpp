#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>

android_app* xiiAndroidUtils::s_pAndroidApplication;
JavaVM*      xiiAndroidUtils::s_pJavaVM;

void xiiAndroidUtils::SetNativeAndroidApp(android_app* pAndroidApp)
{
  s_pAndroidApplication = pAndroidApp;
  SetAndroidJavaVM(pAndroidApp->activity->vm);
}

android_app* xiiAndroidUtils::GetNativeAndroidApp()
{
  return s_pAndroidApplication;
}

void xiiAndroidUtils::SetAndroidJavaVM(JavaVM* pJavaVM)
{
  s_pJavaVM = pJavaVM;
}

JavaVM* xiiAndroidUtils::GetAndroidJavaVM()
{
  return s_pJavaVM;
}

#endif


XII_STATICLINK_FILE(Foundation, Foundation_Basics_Platform_Android_AndroidUtils);
