/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

xiiResult xiiScreen::EnumerateScreens(xiiHybridArray<xiiScreenInfo, 2>& out_Screens)
{
  out_Screens.Clear();

  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    xiiLog::Error("Unable to initialize SDL Video: {}", SDL_GetError());
    return XII_FAILURE;
  }

  xiiInt32       iDisplayCount;
  SDL_DisplayID* pDisplayIDs = SDL_GetDisplays(&iDisplayCount);

  XII_SCOPE_EXIT(if (pDisplayIDs != nullptr) { SDL_free(pDisplayIDs); });

  for (xiiInt32 i = 0; i < iDisplayCount; ++i)
  {
    xiiScreenInfo& screen = out_Screens.ExpandAndGetRef();
    screen.m_sDisplayName = SDL_GetDisplayName(pDisplayIDs[i]);

    if (screen.m_sDisplayName.IsEmpty())
    {
      xiiLog::Error("SDL failed to retrieve display name for display {} with error '{}'.", i, SDL_GetError());
      return XII_FAILURE;
    }

    SDL_Rect displayBounds;
    if (!SDL_GetDisplayBounds(pDisplayIDs[i], &displayBounds))
    {
      xiiLog::Error("SDL failed to retrieve display bounds for display {} with error '{}'.", i, SDL_GetError());
      return XII_FAILURE;
    }

    // SDL reports screen coordinates from top/left to bottom/right.
    screen.m_iOffsetX     = displayBounds.x;
    screen.m_iOffsetY     = displayBounds.y;
    screen.m_iResolutionX = displayBounds.w;
    screen.m_iResolutionY = displayBounds.h;
    screen.m_bIsPrimary   = i == 0; // Primary Monitor at index 0
  }

  if (out_Screens.IsEmpty())
    return XII_FAILURE;

  return XII_SUCCESS;
}
