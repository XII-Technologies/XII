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

  // Initialize the video subsystem if not initialized.
  if (!SDL_WasInit(SDL_INIT_VIDEO))
  {
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
      xiiLog::Error("Failed to initialize the SDL Video Subsystem with error '{0}'.", SDL_GetError());
      return XII_FAILURE;
    }
  }

  // Initialize the event subsystem if not initialized.
  if (!SDL_WasInit(SDL_INIT_EVENTS))
  {
    if (!SDL_InitSubSystem(SDL_INIT_EVENTS))
    {
      xiiLog::Error("Failed to initialize the SDL Event Subsystem with error '{0}'.", SDL_GetError());
      return XII_FAILURE;
    }
  }

  SDL_Window* pMonitor      = nullptr;
  xiiUInt32   uiWindowFlags = 0;

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

  pMonitor = SDL_CreateWindow(m_CreationDescription.m_Title, m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height, uiWindowFlags);
  if (pMonitor == nullptr)
  {
    xiiLog::Error("Failed to initialize SDL Window with error '{}'", SDL_GetError());
    return XII_FAILURE;
  }
  m_hWindowHandle = pMonitor;

  if (m_CreationDescription.m_Position != xiiVec2I32(0x80000000, 0x80000000))
  {
    SDL_SetWindowPosition(m_hWindowHandle, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y);
  }
  if (m_CreationDescription.m_bSetForegroundOnInit)
  {
    SDL_RaiseWindow(m_hWindowHandle);
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
  m_pInputDevice = nullptr;

  SDL_DestroyWindow(m_hWindowHandle);

  m_hWindowHandle = nullptr;

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
    xiiLog::Error("Failed to initialize SDL Window size with error '{}'", SDL_GetError());
  }
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
    switch (event.type)
    {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      case SDL_EVENT_QUIT:
      {
        OnClickClose();
      }
      break;
      case SDL_EVENT_WINDOW_MOVED:
      {
        OnWindowMove(event.window.data1, event.window.data2);
      }
      break;
      case SDL_EVENT_WINDOW_RESIZED:
      {
        OnResize(xiiSizeU32(static_cast<xiiUInt32>(event.window.data1), static_cast<xiiUInt32>(event.window.data2)));
      }
      break;
      case SDL_EVENT_WINDOW_FOCUS_GAINED:
      {
        OnFocus(true);
      }
      break;
      case SDL_EVENT_WINDOW_FOCUS_LOST:
      {
        OnFocus(false);
      }
      break;

      default:
        break;
    }

    if (auto pInputDevice = GetInputDevice())
    {
      pInputDevice->WindowMessage(&event);
    }
  }
}

void xiiWindow::OnResize(const xiiSizeU32& newWindowSize)
{
  xiiLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}

xiiWindowHandle xiiWindow::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}

#endif
