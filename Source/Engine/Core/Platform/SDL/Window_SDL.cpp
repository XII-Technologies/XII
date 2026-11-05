/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#if XII_ENABLED(XII_SUPPORTS_SDL)

#  include <Core/System/Window.h>
#  include <Foundation/Configuration/Startup.h>
#  include <Foundation/System/Screen.h>

#  include <SDL3/SDL_init.h>
#  include <SDL3/SDL_video.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Core, Window)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
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

xiiResult xiiWindow::Initialize()
{
  XII_LOG_BLOCK("xiiWindow::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
  {
    Destroy().IgnoreResult();
  }

  XII_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");

  // Initialize SDL subsystems. If SDL hasn't been initialized at all, call SDL_Init
  // which provides a clearer error state. Otherwise initialize any missing subsystems.

  if (!SDL_WasInit(0))
  {
    // Nothing initialized yet - initialize both video and events together.
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
      const char* szError = SDL_GetError();
      xiiLog::Error("Failed to initialize the SDL subsystems (video/events) with error '{}'.", (szError && szError[0]) ? szError : "<no error>");
      return XII_FAILURE;
    }
  }
  else
  {
    if (!SDL_WasInit(SDL_INIT_VIDEO))
    {
      if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
      {
        const char* szError = SDL_GetError();
        xiiLog::Error("Failed to initialize the SDL Video Subsystem with error '{}'.", (szError && szError[0]) ? szError : "<no error>");
        return XII_FAILURE;
      }
    }

    if (!SDL_WasInit(SDL_INIT_EVENTS))
    {
      if (!SDL_InitSubSystem(SDL_INIT_EVENTS))
      {
        const char* szError = SDL_GetError();
        xiiLog::Error("Failed to initialize the SDL Event Subsystem with error '{}'.", (szError && szError[0]) ? szError : "<no error>");
        return XII_FAILURE;
      }
    }
  }

  SDL_Window* pWindow       = nullptr;
  xiiUInt32   uiWindowFlags = 0U;

  switch (m_CreationDescription.m_WindowMode)
  {
    case xiiWindowMode::WindowFixedResolution:
      break;

    case xiiWindowMode::WindowResizable:
      uiWindowFlags |= SDL_WINDOW_RESIZABLE;
      break;

    case xiiWindowMode::FullscreenBorderlessNativeResolution:
      uiWindowFlags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
      break;

    case xiiWindowMode::FullscreenFixedResolution:
      uiWindowFlags |= SDL_WINDOW_FULLSCREEN;
      break;
  }

  // If the Primary Monitor is selected, the display index will be -1 in which we will use 0 by default.
  xiiUInt32 uiDisplayMonitor = m_CreationDescription.m_iMonitor > 0 ? xiiUInt32(m_CreationDescription.m_iMonitor) : 0U;

  xiiHybridArray<xiiScreenInfo, 2> screens;
  XII_SUCCEED_OR_RETURN(xiiScreen::EnumerateScreens(screens));
  XII_ASSERT_DEBUG(!screens.IsEmpty(), "Implementation error.");

  // Verify that the selected monitor can be used.
  if (uiDisplayMonitor >= screens.GetCount())
  {
    xiiLog::Error("Can not create window on monitor {} only {} monitors connected.", uiDisplayMonitor, screens.GetCount());
    return XII_FAILURE;
  }

  // Setup full-screen mode at fixed resolution.
  if (m_CreationDescription.m_WindowMode == xiiWindowMode::FullscreenFixedResolution)
  {
    xiiLog::Dev("Changing display resolution for Fixed Resolution to {0}*{1}", m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height);

    // Check all possible resolutions and see if it fits any and clamp to the nearest greater supported resolution.
    // The maximum resolution is situated at the first display index.

    xiiUInt32 uiMaxWidth               = screens[0].m_iResolutionX;
    xiiUInt32 uiMaxHeight              = screens[0].m_iResolutionY;
    bool      bSuitableResolutionFound = false;

    // Check other resolutions and get the nearest resolution that is greater than the specified resolution.
    // If a suitable resolution is found that matches the specified resolution, use that instead.
    for (xiiUInt32 i = 1; i < screens.GetCount(); ++i)
    {
      const auto& currentScreen = screens[i];

      if (currentScreen.m_iResolutionX >= static_cast<xiiInt32>(m_CreationDescription.m_Resolution.width) && currentScreen.m_iResolutionY >= static_cast<xiiInt32>(m_CreationDescription.m_Resolution.height))
      {
        uiMaxWidth  = currentScreen.m_iResolutionX;
        uiMaxHeight = currentScreen.m_iResolutionY;

        if (static_cast<xiiInt32>(m_CreationDescription.m_Resolution.width) == currentScreen.m_iResolutionX && static_cast<xiiInt32>(m_CreationDescription.m_Resolution.height) == currentScreen.m_iResolutionY)
        {
          bSuitableResolutionFound = true;
          break;
        }
      }
    }

    if (!bSuitableResolutionFound)
    {
      xiiLog::Dev("Suitable display resolution not found with resolution {0}*{1}, using next higher display resolution {2}*{3}", m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height, uiMaxWidth, uiMaxHeight);

      m_CreationDescription.m_Resolution.width  = uiMaxWidth;
      m_CreationDescription.m_Resolution.height = uiMaxHeight;
    }
  }

  pWindow = SDL_CreateWindow(m_CreationDescription.m_Title, m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height, uiWindowFlags);
  if (pWindow == nullptr)
  {
    xiiLog::Error("Failed to initialize SDL Window with error '{}'", SDL_GetError());
    return XII_FAILURE;
  }

  m_hWindowHandle = pWindow;


  if (m_CreationDescription.m_Position != xiiVec2I32(0x80000000, 0x80000000))
  {
    SDL_SetWindowPosition(m_hWindowHandle, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y);
  }
  if (m_CreationDescription.m_bSetForegroundOnInit)
  {
    SDL_RaiseWindow(m_hWindowHandle);
  }

  if (!SDL_SetPointerProperty(SDL_GetWindowProperties(pWindow), "XII.Window", this))
  {
    SDL_DestroyWindow(pWindow);

    m_hWindowHandle = XII_INVALID_WINDOW_HANDLE_VALUE;

    return XII_FAILURE;
  }

  m_pInputDevice = XII_DEFAULT_NEW(xiiStandardInputDevice, m_CreationDescription.m_uiWindowNumber, m_hWindowHandle);

  m_pInputDevice->SetClipMouseCursor(m_CreationDescription.m_bClipMouseCursor ? xiiMouseCursorClipMode::ClipToWindowImmediate : xiiMouseCursorClipMode::NoClip);
  m_pInputDevice->SetShowMouseCursor(m_CreationDescription.m_bShowMouseCursor);

  m_bInitialized = true;

  xiiLog::Success("Created SDL window successfully. Resolution is {0}*{1}", GetClientAreaSize().width, GetClientAreaSize().height);

  return XII_SUCCESS;
}

xiiResult xiiWindow::Destroy()
{
  if (!m_bInitialized)
    return XII_SUCCESS;

  XII_LOG_BLOCK("xiiWindow::Destroy");

  m_pInputDevice.Clear();

  m_pOutputTarget.Clear();

  SDL_DestroyWindow(m_hWindowHandle);

  m_hWindowHandle = XII_INVALID_WINDOW_HANDLE_VALUE;

  m_bInitialized = false;

  return XII_SUCCESS;
}

xiiResult xiiWindow::Resize(const xiiSizeU32& newWindowSize)
{
  if (!m_bInitialized)
    return XII_FAILURE;

  XII_ASSERT_DEV(newWindowSize.HasNonZeroArea(), "Invalid window size.");

  if (!SDL_SetWindowSize(m_hWindowHandle, newWindowSize.width, newWindowSize.height))
  {
    xiiLog::Error("Failed to resize SDL Window with error '{}'", SDL_GetError());
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

xiiResult xiiWindow::SetTitle(xiiStringView sTitle)
{
  xiiString title(sTitle);
  if (!m_bInitialized || !SDL_SetWindowTitle(m_hWindowHandle, title))
    return XII_FAILURE;
  m_CreationDescription.m_Title = title;
  return XII_SUCCESS;
}

xiiResult xiiWindow::ApplyWindowSettings(const xiiWindowCreationDescription& description)
{
  if (!m_bInitialized || !description.m_Resolution.HasNonZeroArea() || description.m_Resolution.width > 16384 || description.m_Resolution.height > 16384)
    return XII_FAILURE;
  int            displayCount = 0;
  SDL_DisplayID* displays     = SDL_GetDisplays(&displayCount);
  const int      monitor      = description.m_iMonitor < 0 ? 0 : description.m_iMonitor;
  if (!displays || monitor >= displayCount)
  {
    SDL_free(displays);
    return XII_FAILURE;
  }
  const SDL_DisplayID display = description.m_iMonitor < 0 ? SDL_GetPrimaryDisplay() : displays[monitor];
  SDL_free(displays);
  const bool fullscreen = xiiWindowMode::IsFullscreen(description.m_WindowMode);
  if (!SDL_SetWindowFullscreen(m_hWindowHandle, false))
    return XII_FAILURE;
  SDL_Rect bounds;
  if (!SDL_GetDisplayBounds(display, &bounds) ||
      !SDL_SetWindowPosition(m_hWindowHandle, bounds.x, bounds.y) ||
      !SDL_SetWindowResizable(m_hWindowHandle, description.m_WindowMode == xiiWindowMode::WindowResizable))
    return XII_FAILURE;
  if (description.m_WindowMode == xiiWindowMode::FullscreenFixedResolution)
  {
    SDL_DisplayMode mode;
    if (!SDL_GetClosestFullscreenDisplayMode(display, description.m_Resolution.width, description.m_Resolution.height, 0.0f, true, &mode) ||
        !SDL_SetWindowFullscreenMode(m_hWindowHandle, &mode))
      return XII_FAILURE;
  }
  else if (!SDL_SetWindowFullscreenMode(m_hWindowHandle, nullptr))
    return XII_FAILURE;
  if (fullscreen)
  {
    if (!SDL_SetWindowFullscreen(m_hWindowHandle, true))
      return XII_FAILURE;
  }
  else
  {
    if (!SDL_SetWindowSize(m_hWindowHandle, description.m_Resolution.width, description.m_Resolution.height))
      return XII_FAILURE;
    const bool explicitPosition = description.m_Position != xiiVec2I32(0x80000000, 0x80000000);
    const int  x                = explicitPosition && !description.m_bCenterWindowOnDisplay ? description.m_Position.x : bounds.x + (bounds.w - description.m_Resolution.width) / 2;
    const int  y                = explicitPosition && !description.m_bCenterWindowOnDisplay ? description.m_Position.y : bounds.y + (bounds.h - description.m_Resolution.height) / 2;
    if (!SDL_SetWindowPosition(m_hWindowHandle, x, y))
      return XII_FAILURE;
  }
  m_CreationDescription.m_WindowMode             = description.m_WindowMode;
  m_CreationDescription.m_iMonitor               = description.m_iMonitor;
  m_CreationDescription.m_bClipMouseCursor       = description.m_bClipMouseCursor;
  m_CreationDescription.m_bShowMouseCursor       = description.m_bShowMouseCursor;
  m_CreationDescription.m_bCenterWindowOnDisplay = description.m_bCenterWindowOnDisplay;
  m_CreationDescription.m_Position               = description.m_Position;
  return XII_SUCCESS;
}

void xiiWindow::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return;

  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    SDL_Window* pWindowSDL    = SDL_GetWindowFromEvent(&event);
    auto*       pWindowTarget = pWindowSDL ? static_cast<xiiWindow*>(SDL_GetPointerProperty(SDL_GetWindowProperties(pWindowSDL), "XII.Window", nullptr)) : nullptr;

    if (event.type == SDL_EVENT_QUIT)
    {
      pWindowTarget = this;
    }

    if (!pWindowTarget)
      continue;

    if (auto* pInputDevice = pWindowTarget->GetInputDevice())
    {
      pInputDevice->WindowMessage(&event);
    }

    switch (event.type)
    {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      case SDL_EVENT_QUIT:
      {
        pWindowTarget->OnClickClose();
      }
      break;
      case SDL_EVENT_WINDOW_MOVED:
      {
        pWindowTarget->OnWindowMove(event.window.data1, event.window.data2);
      }
      break;
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      case SDL_EVENT_WINDOW_RESIZED:
      {
        int width = 0, height = 0;
        if (!SDL_GetWindowSizeInPixels(pWindowTarget->m_hWindowHandle, &width, &height) || width <= 0 || height <= 0)
          break;
        xiiSizeU32 newSize = {static_cast<xiiUInt32>(width), static_cast<xiiUInt32>(height)};
        if (newSize != pWindowTarget->m_CreationDescription.m_Resolution)
        {
          pWindowTarget->m_CreationDescription.m_Resolution = newSize;

          pWindowTarget->OnResize(newSize);
        }
      }
      break;
      case SDL_EVENT_WINDOW_RESTORED:
      {
        // Window was restored from minimized state. Notify visibility change and update size.
        xiiInt32 iWidth = 0, iHeight = 0;
        if (pWindowTarget->m_hWindowHandle)
        {
          SDL_GetWindowSizeInPixels(pWindowTarget->m_hWindowHandle, &iWidth, &iHeight);
        }

        {
          xiiSizeU32 reportedSize = {static_cast<xiiUInt32>(iWidth), static_cast<xiiUInt32>(iHeight)};
          if (reportedSize != pWindowTarget->m_CreationDescription.m_Resolution)
          {
            pWindowTarget->m_CreationDescription.m_Resolution = reportedSize;

            pWindowTarget->OnResize(reportedSize);
          }
        }
        pWindowTarget->OnVisibleChange(true);
      }
      break;
      case SDL_EVENT_WINDOW_FOCUS_GAINED:
      {
        pWindowTarget->OnFocus(true);
        pWindowTarget->OnVisibleChange(true);
      }
      break;
      case SDL_EVENT_WINDOW_FOCUS_LOST:
      {
        pWindowTarget->OnFocus(false);
      }
      break;
      case SDL_EVENT_WINDOW_MINIMIZED:
      {
        pWindowTarget->OnVisibleChange(false);
      }
      break;
      case SDL_EVENT_WINDOW_MAXIMIZED:
      {
        // Window may have changed size when maximized. Query and notify about the new size.
        xiiInt32 iWidth = 0, iHeight = 0;
        if (pWindowTarget->m_hWindowHandle)
        {
          SDL_GetWindowSizeInPixels(pWindowTarget->m_hWindowHandle, &iWidth, &iHeight);
        }
        {
          xiiSizeU32 reportedSize = {static_cast<xiiUInt32>(iWidth), static_cast<xiiUInt32>(iHeight)};
          if (reportedSize != pWindowTarget->m_CreationDescription.m_Resolution)
          {
            pWindowTarget->m_CreationDescription.m_Resolution = reportedSize;

            pWindowTarget->OnResize(reportedSize);
          }
        }
        pWindowTarget->OnVisibleChange(true);
      }
      break;
      case SDL_EVENT_WINDOW_HIDDEN:
      {
        pWindowTarget->OnVisibleChange(false);
      }
      break;
      case SDL_EVENT_WINDOW_SHOWN:
      {
        // Window shown - ensure the current size is propagated.
        xiiInt32 iWidth = 0, iHeight = 0;
        if (pWindowTarget->m_hWindowHandle)
        {
          SDL_GetWindowSizeInPixels(pWindowTarget->m_hWindowHandle, &iWidth, &iHeight);
        }
        {
          xiiSizeU32 reportedSize = {static_cast<xiiUInt32>(iWidth), static_cast<xiiUInt32>(iHeight)};
          if (reportedSize != pWindowTarget->m_CreationDescription.m_Resolution)
          {
            pWindowTarget->m_CreationDescription.m_Resolution = reportedSize;

            pWindowTarget->OnResize(reportedSize);
          }
        }
        pWindowTarget->OnVisibleChange(true);
      }
      break;

      default:
        break;
    }
  }
}

xiiWindowHandle xiiWindow::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}

#endif

XII_STATICLINK_FILE(Core, Core_Platform_SDL_Window_SDL);
