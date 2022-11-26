#include <Core/System/Window.h>
#include <Foundation/Configuration/Startup.h>

#include <GLFW/glfw3.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  ifdef APIENTRY
#    undef APIENTRY
#endif

# include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  define GLFW_EXPOSE_NATIVE_WIN32
#  include <GLFW/glfw3native.h>
#endif

namespace
{
  void glfwErrorCallback(int errorCode, const char* msg)
  {
    xiiLog::Error("GLFW error {}: {}", errorCode, msg);
  }
}


// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Core, Window)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    if (!glfwInit())
    {
      const char* szErrorDesc = nullptr;
      int iErrorCode = glfwGetError(&szErrorDesc);
      xiiLog::Warning("Failed to initialize glfw. Window and input related functionality will not be available. Error Code {}. GLFW Error Message: {}", iErrorCode, szErrorDesc);
    }
    else
    {
      // Set the error callback after init, so we don't print an error if init fails.
      glfwSetErrorCallback(&glfwErrorCallback);
    }
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    glfwSetErrorCallback(nullptr);
    glfwTerminate();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace {
  xiiResult xiiGlfwError(const char* file, size_t line)
  {
    const char* desc;
    int errorCode = glfwGetError(&desc);
    if(errorCode != GLFW_NO_ERROR)
    {
      xiiLog::Error("GLFW error {} ({}): {} - {}", file, line, errorCode, desc);
      return XII_FAILURE;
    }
    return XII_SUCCESS;
  }
}

#define XII_GLFW_RETURN_FAILURE_ON_ERROR() do { if(xiiGlfwError(__FILE__, __LINE__).Failed()) return XII_FAILURE; } while(false)

xiiResult xiiWindow::Initialize()
{
  XII_LOG_BLOCK("xiiWindow::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
  {
    Destroy().IgnoreResult();
  }

  XII_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");

  GLFWmonitor* pMonitor = nullptr; // nullptr for windowed, fullscreen otherwise

  switch (m_CreationDescription.m_WindowMode)
  {
    case xiiWindowMode::WindowResizable:
      glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
      XII_GLFW_RETURN_FAILURE_ON_ERROR();
      break;
    case xiiWindowMode::WindowFixedResolution:
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
      XII_GLFW_RETURN_FAILURE_ON_ERROR();
      break;
    case xiiWindowMode::FullscreenFixedResolution:
    case xiiWindowMode::FullscreenBorderlessNativeResolution:
      if (m_CreationDescription.m_iMonitor == -1)
      {
        pMonitor = glfwGetPrimaryMonitor();
        XII_GLFW_RETURN_FAILURE_ON_ERROR();
      }
      else
      {
        int iMonitorCount = 0;
        GLFWmonitor** pMonitors = glfwGetMonitors(&iMonitorCount);
        XII_GLFW_RETURN_FAILURE_ON_ERROR();
        if (m_CreationDescription.m_iMonitor >= iMonitorCount)
        {
          xiiLog::Error("Can not create window on monitor {} only {} monitors connected", m_CreationDescription.m_iMonitor, iMonitorCount);
          return XII_FAILURE;
        }
        pMonitor = pMonitors[m_CreationDescription.m_iMonitor];
      }

      if (m_CreationDescription.m_WindowMode == xiiWindowMode::FullscreenBorderlessNativeResolution)
      {
        const GLFWvidmode* pVideoMode = glfwGetVideoMode(pMonitor);
        XII_GLFW_RETURN_FAILURE_ON_ERROR();
        if(pVideoMode == nullptr)
        {
          xiiLog::Error("Failed to get video mode for monitor");
          return XII_FAILURE;
        }
        m_CreationDescription.m_Resolution.width = pVideoMode->width;
        m_CreationDescription.m_Resolution.height = pVideoMode->height;
        m_CreationDescription.m_Position.x = 0;
        m_CreationDescription.m_Position.y = 0;

        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        XII_GLFW_RETURN_FAILURE_ON_ERROR();
      }

      break;
  }


  glfwWindowHint(GLFW_FOCUS_ON_SHOW, m_CreationDescription.m_bSetForegroundOnInit ? GLFW_TRUE : GLFW_FALSE);
  XII_GLFW_RETURN_FAILURE_ON_ERROR();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  XII_GLFW_RETURN_FAILURE_ON_ERROR();

  GLFWwindow* pWindow = glfwCreateWindow(m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height, m_CreationDescription.m_Title.GetData(), pMonitor, NULL);
  XII_GLFW_RETURN_FAILURE_ON_ERROR();

  if (pWindow == nullptr)
  {
    xiiLog::Error("Failed to create glfw window");
    return XII_FAILURE;
  }
#if XII_ENABLED(XII_PLATFORM_LINUX)
  m_hWindowHandle.type = xiiWindowHandle::Type::GLFW;
  m_hWindowHandle.glfwWindow = pWindow;
#else
  m_hWindowHandle = pWindow;
#endif

  if (m_CreationDescription.m_Position != xiiVec2I32(0x80000000, 0x80000000))
  {
    glfwSetWindowPos(pWindow, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y);
    XII_GLFW_RETURN_FAILURE_ON_ERROR();
  }

  glfwSetWindowUserPointer(pWindow, this);
  glfwSetWindowSizeCallback(pWindow, &xiiWindow::SizeCallback);
  glfwSetWindowPosCallback(pWindow, &xiiWindow::PositionCallback);
  glfwSetWindowCloseCallback(pWindow, &xiiWindow::CloseCallback);
  glfwSetWindowFocusCallback(pWindow, &xiiWindow::FocusCallback);
  glfwSetKeyCallback(pWindow, &xiiWindow::KeyCallback);
  glfwSetCharCallback(pWindow, &xiiWindow::CharacterCallback);
  glfwSetCursorPosCallback(pWindow, &xiiWindow::CursorPositionCallback);
  glfwSetMouseButtonCallback(pWindow, &xiiWindow::MouseButtonCallback);
  glfwSetScrollCallback(pWindow, &xiiWindow::ScrollCallback);
  XII_GLFW_RETURN_FAILURE_ON_ERROR();

#if XII_ENABLED(XII_PLATFORM_LINUX)
  XII_ASSERT_DEV(m_hWindowHandle.type == xiiWindowHandle::Type::GLFW, "not a GLFW handle");
  m_pInputDevice = XII_DEFAULT_NEW(xiiStandardInputDevice, m_CreationDescription.m_uiWindowNumber, m_hWindowHandle.glfwWindow);
#else
  m_pInputDevice = XII_DEFAULT_NEW(xiiStandardInputDevice, m_CreationDescription.m_uiWindowNumber, m_hWindowHandle);
#endif

  m_pInputDevice->SetClipMouseCursor(m_CreationDescription.m_bClipMouseCursor ? xiiMouseCursorClipMode::ClipToWindowImmediate : xiiMouseCursorClipMode::NoClip);
  m_pInputDevice->SetShowMouseCursor(m_CreationDescription.m_bShowMouseCursor);

  m_bInitialized = true;
  xiiLog::Success("Created glfw window successfully. Resolution is {0}*{1}", GetClientAreaSize().width, GetClientAreaSize().height);

  return XII_SUCCESS;
}

xiiResult xiiWindow::Destroy()
{
  if (m_bInitialized)
  {
    XII_LOG_BLOCK("xiiWindow::Destroy");

    m_pInputDevice = nullptr;

#if XII_ENABLED(XII_PLATFORM_LINUX)
    XII_ASSERT_DEV(m_hWindowHandle.type == xiiWindowHandle::Type::GLFW, "GLFW handle expected");
    glfwDestroyWindow(m_hWindowHandle.glfwWindow);
#else
    glfwDestroyWindow(m_hWindowHandle);
#endif
    m_hWindowHandle = INVALID_INTERNAL_WINDOW_HANDLE_VALUE;

    m_bInitialized = false;
  }

  return XII_SUCCESS;
}

xiiResult xiiWindow::Resize(const xiiSizeU32& newWindowSize)
{
  if (!m_bInitialized)
    return XII_FAILURE;

#if XII_ENABLED(XII_PLATFORM_LINUX)
  XII_ASSERT_DEV(m_hWindowHandle.type == xiiWindowHandle::Type::GLFW, "Expected GLFW handle");
  glfwSetWindowSize(m_hWindowHandle.glfwWindow, newWindowSize.width, newWindowSize.height);
#else
  glfwSetWindowSize(m_hWindowHandle, newWindowSize.width, newWindowSize.height);
#endif
  XII_GLFW_RETURN_FAILURE_ON_ERROR();

  return XII_SUCCESS;
}

void xiiWindow::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return;

  // Only run the global event processing loop for the main window.
  if (m_CreationDescription.m_uiWindowNumber == 0)
  {
    glfwPollEvents();
  }

#if XII_ENABLED(XII_PLATFORM_LINUX)
  XII_ASSERT_DEV(m_hWindowHandle.type == xiiWindowHandle::Type::GLFW, "Expected GLFW handle");
  if (glfwWindowShouldClose(m_hWindowHandle.glfwWindow))
  {
    Destroy().IgnoreResult();
  }
#else
  if (glfwWindowShouldClose(m_hWindowHandle))
  {
    Destroy().IgnoreResult();
  }
#endif
}

void xiiWindow::OnResize(const xiiSizeU32& newWindowSize)
{
  xiiLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}

void xiiWindow::SizeCallback(GLFWwindow* window, int width, int height)
{
  auto self = static_cast<xiiWindow*>(glfwGetWindowUserPointer(window));
  if (self && width > 0 && height > 0)
  {
    self->OnResize(xiiSizeU32(static_cast<xiiUInt32>(width), static_cast<xiiUInt32>(height)));
  }
}

void xiiWindow::PositionCallback(GLFWwindow* window, int xpos, int ypos)
{
  auto self = static_cast<xiiWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnWindowMove(xpos, ypos);
  }
}

void xiiWindow::CloseCallback(GLFWwindow* window)
{
  auto self = static_cast<xiiWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnClickClose();
  }
}

void xiiWindow::FocusCallback(GLFWwindow* window, int focused)
{
  auto self = static_cast<xiiWindow*>(glfwGetWindowUserPointer(window));
  if (self)
  {
    self->OnFocus(focused ? true : false);
  }
}

void xiiWindow::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  auto self = static_cast<xiiWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnKey(key, scancode, action, mods);
  }
}

void xiiWindow::CharacterCallback(GLFWwindow* window, unsigned int codepoint)
{
  auto self = static_cast<xiiWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnCharacter(codepoint);
  }
}

void xiiWindow::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
  auto self = static_cast<xiiWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnCursorPosition(xpos, ypos);
  }
}

void xiiWindow::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
  auto self = static_cast<xiiWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnMouseButton(button, action, mods);
  }
}

void xiiWindow::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
  auto self = static_cast<xiiWindow*>(glfwGetWindowUserPointer(window));
  if (self && self->m_pInputDevice)
  {
    self->m_pInputDevice->OnScroll(xoffset, yoffset);
  }
}

xiiWindowHandle xiiWindow::GetNativeWindowHandle() const
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
  return xiiMinWindows::FromNative<HWND>(glfwGetWin32Window(m_hWindowHandle));
#else
  return m_hWindowHandle;
#endif
}
