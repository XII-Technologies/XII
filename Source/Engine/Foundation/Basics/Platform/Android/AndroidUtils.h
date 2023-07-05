#pragma once

#if XII_DISABLED(XII_PLATFORM_ANDROID)
#  error "The Android utility header should only be included in Android builds!"
#endif

struct android_app;

struct _JavaVM;
using JavaVM = _JavaVM;

struct _JNIEnv;
using JNIEnv = _JNIEnv;

class _jobject;
using JObject = _jobject*;

class XII_FOUNDATION_DLL xiiAndroidUtils
{
public:
  static void         SetNativeAndroidApp(android_app* pAndroidApp);
  static android_app* GetNativeAndroidApp();

  static void    SetAndroidJavaVM(JavaVM* pJavaVM);
  static JavaVM* GetAndroidJavaVM();

  static void    SetAndroidNativeActivity(JObject pNativeActivity);
  static JObject GetAndroidNativeActivity();

private:
  static android_app* s_pAndroidApplication;
  static JavaVM*      s_pJavaVM;
  static jobject      s_pNativeActivity;
};
