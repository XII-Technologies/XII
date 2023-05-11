#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>

AndroidApplication* xiiAndroidUtils::s_pAndroidApplication;
JavaVM*             xiiAndroidUtils::s_pJavaVM;

void xiiAndroidUtils::SetAndroidApp(AndroidApplication* pAndroidApp)
{
  s_pAndroidApplication = pAndroidApp;
  SetAndroidJavaVM(pAndroidApp->activity->vm);
}

AndroidApplication* xiiAndroidUtils::GetAndroidApplication()
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
