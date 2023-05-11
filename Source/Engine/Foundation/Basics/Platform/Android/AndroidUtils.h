#pragma once

#if XII_DISABLED(XII_PLATFORM_ANDROID)
#  error "The Android utility header should only be included in Android builds!"
#endif

struct android_app;
using AndroidApplication = android_app;

struct _JavaVM;
using JavaVM = _JavaVM;

struct _JNIEnv;
using JNIEnv = _JNIEnv;

class XII_FOUNDATION_DLL xiiAndroidUtils
{
public:
  static void                SetAndroidAppplication(AndroidApplication* pAndroidApp);
  static AndroidApplication* GetAndroidApplication();

  static void    SetAndroidJavaVM(JavaVM* pJavaVM);
  static JavaVM* GetAndroidJavaVM();

private:
  static AndroidApplication* s_pAndroidApplication;
  static JavaVM*             s_pJavaVM;
};
