#include <Foundation/FoundationPCH.h>

#include <Foundation/System/PlatformFeatures.h>
#include <Foundation/System/Screen.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/System/Implementation/Win/Screen_win32.inl>
#elif XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  include <Foundation/System/Implementation/uwp/Screen_uwp.inl>
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/System/Implementation/Android/Screen_android.inl>
#else

xiiResult xiiScreen::EnumerateScreens(xiiHybridArray<xiiScreenInfo, 2>& out_Screens)
{
  XII_ASSERT_NOT_IMPLEMENTED;
  return XII_FAILURE;
}

#endif

void xiiScreen::PrintScreenInfo(const xiiHybridArray<xiiScreenInfo, 2>& screens, xiiLogInterface* pLog /*= xiiLog::GetThreadLocalLogSystem()*/)
{
  XII_LOG_BLOCK(pLog, "Screens");

  xiiLog::Info(pLog, "Found {0} screens", screens.GetCount());

  for (const auto& screen : screens)
  {
    xiiLog::Dev(pLog, "'{0}': Offset = ({1}, {2}), Resolution = ({3}, {4}){5}", screen.m_sDisplayName, screen.m_iOffsetX, screen.m_iOffsetY, screen.m_iResolutionX, screen.m_iResolutionY, screen.m_bIsPrimary ? " (primary)" : "");
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_System_Implementation_Screen);
