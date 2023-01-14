
#include <Core/System/Window.h>
#include <Foundation/Configuration/Startup.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  ifdef APIENTRY
#    undef APIENTRY
#  endif

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Basics/Platform/Linux/Platform_Linux.h>
#elif XII_ENABLED(XII_PLATFORM_OSX)
#  include <Foundation/Basics/Platform/OSX/Platform_OSX.h>
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidJni.h>
#endif

#include <SDL/include/SDL.h>
#include <SDL/include/SDL_syswm.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Core, Window)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    int code = SDL_Init(SDL_INIT_EVERYTHING);
    if (code < 0)
      xiiLog::Warning("Failed to initialize SDL. Window and input related functionality will not be available. Error Code {}. SDL Error Message: {}", code, SDL_GetError());
  }
  
  ON_CORESYSTEMS_SHUTDOWN
  {
    SDL_Quit();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  xiiResult xiiSDLError(xiiInt32 iReturnCode, const char* file, xiiUInt64 uiLine)
  {
    if (iReturnCode < 0)
    {
      const char* lastError = SDL_GetError();
      xiiLog::Error("SDL error {} ({}): {} - {}", file, uiLine, iReturnCode, lastError);
      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }
} // namespace

#define XII_SDL_RETURN_FAILURE_ON_ERROR(code)                               \
  do {                                                                      \
    if (xiiSDLError(code, __FILE__, __LINE__).Failed()) return XII_FAILURE; \
  } while (false)

xiiResult xiiWindow::Initialize()
{
  XII_LOG_BLOCK("xiiWindow::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
  {
    Destroy().IgnoreResult();
  }

  XII_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");

  SDL_Window* pMonitor    = nullptr;
  xiiUInt32   windowFlags = 0;

  switch (m_CreationDescription.m_WindowMode)
  {
    case xiiWindowMode::WindowFixedResolution:
    {
      windowFlags |= SDL_WINDOW_SHOWN;
    }
    break;

    case xiiWindowMode::WindowResizable:
    {
      windowFlags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
    }
    break;

    case xiiWindowMode::FullscreenBorderlessNativeResolution:
    {
      windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN;
    }
    break;

    case xiiWindowMode::FullscreenFixedResolution:
    {
      windowFlags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN;
    }
    break;
  }

  // If the Primary Monitor is selected, the display index will be -1 in which we will use 0 by default.
  xiiInt8 iDisplayMonitor = 0;
  {
    if (m_CreationDescription.m_iMonitor > 0)
    {
      iDisplayMonitor = m_CreationDescription.m_iMonitor;
    }

    // Verify Monitor
    xiiInt32 iDisplayMonitorsCount = SDL_GetNumVideoDisplays();
    XII_SDL_RETURN_FAILURE_ON_ERROR(iDisplayMonitorsCount);

    if (m_CreationDescription.m_iMonitor >= iDisplayMonitorsCount)
    {
      xiiLog::Error("Can not create window on monitor {} only {} monitors connected.", m_CreationDescription.m_iMonitor, iDisplayMonitorsCount);
      return XII_FAILURE;
    }
  }

  // Setup fullscreen mode at fixed resolution
  if (m_CreationDescription.m_WindowMode == xiiWindowMode::FullscreenFixedResolution)
  {
    xiiLog::Dev("Changing display resolution for Fixed Resolution to {0}*{1}", m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height);

    // Query all possible resolutions and see if it fits any and clamp to the nearest greater supported resolution.
    xiiInt32 iNumDisplayModes = SDL_GetNumDisplayModes(iDisplayMonitor);

    // The maximum resolution is situated at the first display index.
    xiiInt32 iMaxWidth                = m_CreationDescription.m_Resolution.width;
    xiiInt32 iMaxHeight               = m_CreationDescription.m_Resolution.height;
    bool     bSuitableResolutionFound = false;

    {
      SDL_DisplayMode displayMode = {SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0};
      XII_SDL_RETURN_FAILURE_ON_ERROR(SDL_GetDisplayMode(iDisplayMonitor, 0, &displayMode));
      {
        iMaxWidth  = displayMode.w;
        iMaxHeight = displayMode.h;
      }
    }

    // Check other resolutions and get the nearest resolution that is greater than the specified resolution
    // If a suitable resolution is found that matches the specified resolution, use that instead.
    for (xiiInt32 iResIndex = 1; iResIndex < iNumDisplayModes; ++iResIndex)
    {
      SDL_DisplayMode displayMode = {SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0};
      XII_SDL_RETURN_FAILURE_ON_ERROR(SDL_GetDisplayMode(iDisplayMonitor, iResIndex, &displayMode));
      {
        if (displayMode.w >= static_cast<xiiInt32>(m_CreationDescription.m_Resolution.width) && displayMode.h >= static_cast<xiiInt32>(m_CreationDescription.m_Resolution.height))
        {
          iMaxWidth  = displayMode.w;
          iMaxHeight = displayMode.h;

          if (static_cast<xiiInt32>(m_CreationDescription.m_Resolution.width) == displayMode.w && static_cast<xiiInt32>(m_CreationDescription.m_Resolution.height) == displayMode.h)
          {
            bSuitableResolutionFound = true;
            break;
          }
        }
      }
    }

    if (!bSuitableResolutionFound)
    {
      xiiLog::Dev("Suitable display resolution not found with resolution {0}*{1}, using next higher display resolution {2}*{3}", m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height, iMaxWidth, iMaxHeight);
      m_CreationDescription.m_Resolution.width  = iMaxWidth;
      m_CreationDescription.m_Resolution.height = iMaxHeight;
    }
  }

  pMonitor = SDL_CreateWindow(m_CreationDescription.m_Title, SDL_WINDOWPOS_CENTERED_MASK | (iDisplayMonitor), SDL_WINDOWPOS_CENTERED_MASK | (iDisplayMonitor), m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height, windowFlags);
  if (pMonitor == nullptr)
  {
    xiiLog::Error("Failed to iniailize SDL Window with error '{}'", SDL_GetError());
    return XII_FAILURE;
  }

#if XII_ENABLED(XII_PLATFORM_LINUX)
  m_hWindowHandle.type      = xiiWindowHandle::Type::SDL;
  m_hWindowHandle.sdlWindow = pMonitor;
#else
  m_hWindowHandle = pMonitor;
#endif

  if (m_CreationDescription.m_Position != xiiVec2I32(0x80000000, 0x80000000))
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    SDL_SetWindowPosition(m_hWindowHandle, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y);
#elif XII_ENABLED(XII_PLATFORM_LINUX)
    SDL_SetWindowPosition(m_hWindowHandle.sdlWindow, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y);
#else
#  error Platform implementation not available
#endif
  }

  if (m_CreationDescription.m_bSetForegroundOnInit)
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    SDL_RaiseWindow(m_hWindowHandle);
#elif XII_ENABLED(XII_PLATFORM_LINUX)
    SDL_RaiseWindow(m_hWindowHandle.sdlWindow);
#else
#  error Platform implementation not available
#endif
  }

#if XII_ENABLED(XII_PLATFORM_LINUX)
  XII_ASSERT_DEV(m_hWindowHandle.type == xiiWindowHandle::Type::SDL, "Not a SDL handle");
  m_pInputDevice = XII_DEFAULT_NEW(xiiStandardInputDevice, m_CreationDescription.m_uiWindowNumber, m_hWindowHandle.sdlWindow);
#else
  m_pInputDevice = XII_DEFAULT_NEW(xiiStandardInputDevice, m_CreationDescription.m_uiWindowNumber, m_hWindowHandle);
#endif

  m_pInputDevice->SetClipMouseCursor(m_CreationDescription.m_bClipMouseCursor ? xiiMouseCursorClipMode::ClipToWindowImmediate : xiiMouseCursorClipMode::NoClip);
  m_pInputDevice->SetShowMouseCursor(m_CreationDescription.m_bShowMouseCursor);

  m_bInitialized = true;
  xiiLog::Success("Created SDL window successfully. Resolution is {0}*{1}", GetClientAreaSize().width, GetClientAreaSize().height);

  return XII_SUCCESS;
}

xiiResult xiiWindow::Destroy()
{
  if (m_bInitialized)
  {
    XII_LOG_BLOCK("xiiWindow::Destroy");

    m_pInputDevice.Clear();
    m_pInputDevice = nullptr;

#if XII_ENABLED(XII_PLATFORM_LINUX)
    XII_ASSERT_DEV(m_hWindowHandle.type == xiiWindowHandle::Type::SDL, "SDL Handle Expected");
    SDL_DestroyWindow(m_hWindowHandle.sdlWindow);
    m_hWindowHandle = INVALID_WINDOW_HANDLE_VALUE;
#else
    SDL_DestroyWindow(m_hWindowHandle);
    m_hWindowHandle = nullptr;
#endif

    m_bInitialized = false;
  }

  return XII_SUCCESS;
}

xiiResult xiiWindow::Resize(const xiiSizeU32& newWindowSize)
{
  if (!m_bInitialized)
    return XII_FAILURE;

#if XII_ENABLED(XII_PLATFORM_LINUX)
  XII_ASSERT_DEV(m_hWindowHandle.type == xiiWindowHandle::Type::SDL, "SDL Handle Expected");
  SDL_SetWindowSize(m_hWindowHandle.sdlWindow, newWindowSize.width, newWindowSize.height);
#else
  SDL_SetWindowSize(m_hWindowHandle, newWindowSize.width, newWindowSize.height);
#endif

  return XII_SUCCESS;
}

void xiiWindow::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return;

  // Only run the global event processing loop for the main window.
  if (m_CreationDescription.m_uiWindowNumber != 0)
    return;

  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    if (event.type == SDL_QUIT)
    {
      OnClickClose();
    }
    else if (event.type == SDL_WINDOWEVENT)
    {
      switch (event.window.event)
      {
        case SDL_WINDOWEVENT_MOVED:
        {
          OnWindowMove(event.window.data1, event.window.data2);
        }
        break;

        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        {
          OnResize(xiiSizeU32(static_cast<xiiUInt32>(event.window.data1), static_cast<xiiUInt32>(event.window.data2)));
        }
        break;

        case SDL_WINDOWEVENT_FOCUS_GAINED:
        {
          OnFocus(true);
        }
        break;

        case SDL_WINDOWEVENT_FOCUS_LOST:
        {
          OnFocus(false);
        }
        break;

        case SDL_WINDOWEVENT_CLOSE:
        {
          OnClickClose();
        }
        break;
      }
    }

    if (GetInputDevice())
      GetInputDevice()->WindowMessage(&event);
  }
}

void xiiWindow::OnResize(const xiiSizeU32& newWindowSize)
{
  xiiLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}

xiiWindowHandle xiiWindow::GetNativeWindowHandle() const
{
  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  xiiInt32 iReturnCode = SDL_GetWindowWMInfo(m_hWindowHandle, &wmInfo);
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  xiiInt32 iReturnCode = SDL_GetWindowWMInfo(m_hWindowHandle.sdlWindow, &wmInfo);
#else
#  error Platform implementation not available
#endif

  if (iReturnCode == SDL_TRUE)
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
    return xiiMinWindows::FromNative<HWND>(wmInfo.info.win.window);
#elif XII_ENABLED(XII_PLATFORM_LINUX)
    xiiWindowHandle hWindowHandle;
    hWindowHandle.type      = xiiWindowHandle::Type::XCB;
    hWindowHandle.x11Window = wmInfo.info.x11.window;
    return hWindowHandle;
#else
    return m_hWindowHandle;
#endif
  }
  else
  {
    xiiSDLError(iReturnCode, __FILE__, __LINE__).IgnoreResult();
    return INVALID_WINDOW_HANDLE_VALUE;
  }
}
