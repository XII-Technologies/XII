#  include <Core/CorePCH.h>

#if XII_ENABLED(XII_PLATFORM_ANDROID)

#  include <Core/System/Window.h>
#  include <Foundation/Basics.h>
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Types/UniquePtr.h>
#  include <android_native_app_glue.h>

struct ANativeWindow;

namespace
{
  ANativeWindow*         s_androidWindow    = nullptr;
  xiiEventSubscriptionID s_androidCommandID = 0;
} // namespace

xiiResult xiiWindow::Initialize()
{
  XII_LOG_BLOCK("xiiWindow::Initialize", m_CreationDescription.m_Title.GetData());
  if (m_bInitialized)
  {
    Destroy().IgnoreResult();
  }

  if (m_CreationDescription.m_WindowMode == xiiWindowMode::WindowResizable)
  {
    s_androidCommandID = xiiAndroidUtils::s_AppCommandEvent.AddEventHandler([this](xiiInt32 iCmd) {
      if (iCmd == APP_CMD_WINDOW_RESIZED)
      {
        xiiHybridArray<xiiScreenInfo, 2> screens;
        if (xiiScreen::EnumerateScreens(screens).Succeeded())
        {
          m_CreationDescription.m_Resolution.width  = screens[0].m_iResolutionX;
          m_CreationDescription.m_Resolution.height = screens[0].m_iResolutionY;
          this->OnResize(xiiSizeU32(screens[0].m_iResolutionX, screens[0].m_iResolutionY));
        }
      }
    });
  }

  // Checking and adjustments to creation desc.
  if (m_CreationDescription.AdjustWindowSizeAndPosition().Failed())
    xiiLog::Warning("Failed to adjust window size and position settings.");

  XII_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");
  XII_ASSERT_RELEASE(s_androidWindow == nullptr, "Window already exists. Only one Android window is supported at any time!");

  s_androidWindow = xiiAndroidUtils::GetAndroidApp()->window;
  m_hWindowHandle = s_androidWindow;
  m_pInputDevice  = XII_DEFAULT_NEW(xiiStandardInputDevice, 0);
  m_bInitialized  = true;

  return XII_SUCCESS;
}

xiiResult xiiWindow::Destroy()
{
  if (!m_bInitialized)
    return XII_SUCCESS;

  XII_LOG_BLOCK("xiiWindow::Destroy");

  s_androidWindow = nullptr;

  if (s_androidCommandID != 0)
  {
    xiiAndroidUtils::s_AppCommandEvent.RemoveEventHandler(s_androidCommandID);
  }

  xiiLog::Success("Window destroyed.");

  return XII_SUCCESS;
}

xiiResult xiiWindow::Resize(const xiiSizeU32& newWindowSize)
{
  // No need to resize on Android, swapchain can take any size at any time.
  m_CreationDescription.m_Resolution.width  = newWindowSize.width;
  m_CreationDescription.m_Resolution.height = newWindowSize.height;
  return XII_SUCCESS;
}

void xiiWindow::ProcessWindowMessages()
{
  XII_ASSERT_RELEASE(s_androidWindow != nullptr, "No android window data available.");
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
