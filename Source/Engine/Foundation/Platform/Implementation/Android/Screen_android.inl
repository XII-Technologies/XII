#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Android/AndroidUtils.h>

#include <android_native_app_glue.h>

xiiResult xiiScreen::EnumerateScreens(xiiHybridArray<xiiScreenInfo, 2>& out_Screens)
{
  out_Screens.Clear();

  // Currently, Android devices support only one window.
  // Screen coordinates are reported from top/left to bottom/right.
  ANativeWindow* pWindowSurface = xiiAndroidUtils::GetNativeAndroidApp()->window;
  XII_ASSERT_DEV(pWindowSurface != nullptr, "Failed to retrieve window surface. Android application may not have been initialized properly.");

  xiiInt32 iWidth  = ANativeWindow_getWidth(pWindowSurface);
  xiiInt32 iHeight = ANativeWindow_getHeight(pWindowSurface);

  // A negative value is returned on error.
  if (iWidth < 0 || iHeight < 0)
    return XII_FAILURE;

  xiiScreenInfo& screen = out_Screens.ExpandAndGetRef();
  screen.m_sDisplayName = "Current Display";
  screen.m_iOffsetX     = 0;
  screen.m_iOffsetY     = 0;
  screen.m_iResolutionX = ANativeWindow_getWidth(pWindowSurface);
  screen.m_iResolutionY = ANativeWindow_getHeight(pWindowSurface);
  screen.m_bIsPrimary   = true;

  return XII_SUCCESS;
}
