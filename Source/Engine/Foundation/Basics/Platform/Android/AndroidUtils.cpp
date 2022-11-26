#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>

android_app* xiiAndroidUtils::s_app;
JavaVM*      xiiAndroidUtils::s_vm;

void xiiAndroidUtils::SetAndroidApp(android_app* app)
{
  s_app = app;
  SetAndroidJavaVM(s_app->activity->vm);
}

android_app* xiiAndroidUtils::GetAndroidApp()
{
  return s_app;
}

void xiiAndroidUtils::SetAndroidJavaVM(JavaVM* vm)
{
  s_vm = vm;
}

JavaVM* xiiAndroidUtils::GetAndroidJavaVM()
{
  return s_vm;
}

#endif


XII_STATICLINK_FILE(Foundation, Foundation_Basics_Platform_Android_AndroidUtils);
