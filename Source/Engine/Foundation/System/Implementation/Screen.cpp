/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/System/Screen.h>

#if XII_ENABLED(XII_SUPPORTS_SDL)
#  include <Foundation/Platform/Implementation/SDL/Screen_SDL.inl>
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

  xiiLog::Dev(pLog, "Found {0} screens", screens.GetCount());

  for (const auto& screen : screens)
  {
    xiiLog::Dev(pLog, "'{0}': Offset = ({1}, {2}), Resolution = ({3}, {4}){5}", screen.m_sDisplayName, screen.m_iOffsetX, screen.m_iOffsetY, screen.m_iResolutionX, screen.m_iResolutionY, screen.m_bIsPrimary ? " (primary)" : "");
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_System_Implementation_Screen);
