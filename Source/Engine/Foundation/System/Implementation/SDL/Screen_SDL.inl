#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <SDL2/include/SDL.h>

namespace
{
  xiiResult xiiSDLError(xiiInt32 iReturnCode, const char* file, xiiUInt64 uiLine)
  {
    if (iReturnCode > -1)
      return XII_SUCCESS;

    const char* lastError = SDL_GetError();
    xiiLog::Error("SDL error {} ({}): {} - {}", file, uiLine, iReturnCode, lastError);
    return XII_FAILURE;
  }
} // namespace

#define XII_SDL_RETURN_FAILURE_ON_ERROR(code)                               \
  do {                                                                      \
    if (xiiSDLError(code, __FILE__, __LINE__).Failed()) return XII_FAILURE; \
  } while (false)

xiiResult xiiScreen::EnumerateScreens(xiiHybridArray<xiiScreenInfo, 2>& out_Screens)
{
  out_Screens.Clear();

  xiiInt32 iMonitorCount = SDL_GetNumVideoDisplays();
  XII_SDL_RETURN_FAILURE_ON_ERROR(iMonitorCount);

  for (xiiInt32 i = 0; i < iMonitorCount; ++i)
  {
    xiiScreenInfo& screen = out_Screens.ExpandAndGetRef();
    screen.m_sDisplayName = SDL_GetDisplayName(i);

    if (screen.m_sDisplayName == nullptr)
    {
      xiiLog::Error("SDL failed to get display name with error '{}'", SDL_GetError());
      return XII_FAILURE;
    }

    SDL_DisplayMode displayMode;
    xiiInt32        diplayResult = SDL_GetDisplayMode(i, 0, &displayMode);
    XII_SDL_RETURN_FAILURE_ON_ERROR(diplayResult);

    // SDL reports screen coordinates from top/left to bottom/right.
    // ie. 0,0 is left/top , resx/resy is right/bottom
    screen.m_iOffsetX     = 0;
    screen.m_iOffsetY     = 0;
    screen.m_iResolutionX = displayMode.w;
    screen.m_iResolutionY = displayMode.h;
    screen.m_bIsPrimary   = i == 0; // Primary Monitor at index 0
  }

  if (out_Screens.IsEmpty())
    return XII_FAILURE;

  return XII_SUCCESS;
}
