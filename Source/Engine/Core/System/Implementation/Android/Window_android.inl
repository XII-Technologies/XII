#include <Core/CorePCH.h>

#include <Core/System/Window.h>
#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

#include <Foundation/Basics/Platform/Android/AndroidUtils.h>

#include <android/window.h>
#include <android_native_app_glue.h>

xiiResult xiiWindow::Initialize()
{
  XII_LOG_BLOCK("xiiWindow::Initialize", m_CreationDescription.m_Title.GetData());

  XII_ASSERT_RELEASE(s_uiNextUnusedWindowNumber == 1, "You may have at most one window on Android devices.");

  if (m_bInitialized)
  {
    Destroy().IgnoreResult();
  }

  XII_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");

  m_hWindowHandle = xiiAndroidUtils::GetNativeAndroidApp()->window;

  xiiUInt32 uiEnabledFlags  = AWINDOW_FLAG_KEEP_SCREEN_ON | AWINDOW_FLAG_TURN_SCREEN_ON | AWINDOW_FLAG_DISMISS_KEYGUARD;
  xiiUInt32 uiDisabledFlags = 0;

  if (m_CreationDescription.m_WindowMode == xiiWindowMode::FullscreenFixedResolution || m_CreationDescription.m_WindowMode == xiiWindowMode::FullscreenBorderlessNativeResolution)
  {
    uiEnabledFlags |= AWINDOW_FLAG_FULLSCREEN;
  }

  ANativeActivity_setWindowFlags(xiiAndroidUtils::GetNativeAndroidApp()->activity, uiEnabledFlags, uiDisabledFlags);
  ANativeActivity_setWindowFormat(xiiAndroidUtils::GetNativeAndroidApp()->activity, WINDOW_FORMAT_RGBA_8888);

  if (m_CreationDescription.m_Position != xiiVec2I32(0x80000000, 0x80000000))
  {
    xiiLog::SeriousWarning("Custom window positions are unsupported on Android.");

    // \todo
  }

  if (m_CreationDescription.m_bSetForegroundOnInit)
  {
    // \todo
  }

  m_pInputDevice = XII_DEFAULT_NEW(xiiStandardInputDevice, m_CreationDescription.m_uiWindowNumber);

  // Register input device callback
  xiiAndroidUtils::GetNativeAndroidApp()->userData     = this;
  xiiAndroidUtils::GetNativeAndroidApp()->onAppCmd     = xiiWindow::CommandCallback;
  xiiAndroidUtils::GetNativeAndroidApp()->onInputEvent = xiiWindow::InputEventCallback;

  m_bInitialized = true;
  xiiLog::Success("Created Android window successfully. Resolution is {0}*{1}", GetClientAreaSize().width, GetClientAreaSize().height);

  m_pInputDevice = XII_DEFAULT_NEW(xiiStandardInputDevice, m_CreationDescription.m_uiWindowNumber);
  m_pInputDevice->SetClipMouseCursor(m_CreationDescription.m_bClipMouseCursor ? xiiMouseCursorClipMode::ClipToWindowImmediate : xiiMouseCursorClipMode::NoClip);
  m_pInputDevice->SetShowMouseCursor(m_CreationDescription.m_bShowMouseCursor);

  return XII_SUCCESS;
}

xiiResult xiiWindow::Destroy()
{
  if (!m_bInitialized)
    return XII_SUCCESS;

  XII_LOG_BLOCK("xiiWindow::Destroy");

  m_pInputDevice.Clear();
  m_pInputDevice = nullptr;

  m_bInitialized = false;

  return XII_SUCCESS;
}

xiiResult xiiWindow::Resize(const xiiSizeU32& newWindowSize)
{
  xiiLog::SeriousWarning("xiiWindow::Resize is unsupported on android.");

  // \todo

  return XII_FAILURE;
}

void xiiWindow::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return;
}

void xiiWindow::OnResize(const xiiSizeU32& newWindowSize)
{
  xiiLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}

void xiiWindow::CommandCallback(android_app* pAndroidApp, xiiInt32 iCommand)
{
  xiiWindow* pWindow = reinterpret_cast<xiiWindow*>(pAndroidApp->userData);
  if (pWindow == nullptr && !pWindow->IsInitialized())
    return;

  if (pWindow->GetInputDevice())
    pWindow->GetInputDevice()->WindowMessage(pAndroidApp, iCommand);

  switch (iCommand)
  {
    case APP_CMD_START:
    {
    }
    break;
    case APP_CMD_RESUME:
    {
    }
    break;
    case APP_CMD_PAUSE:
    {
    }
    break;
    case APP_CMD_STOP:
    {
    }
    break;
    case APP_CMD_DESTROY:
    {
      pWindow->OnClickClose();
    }
    break;
    case APP_CMD_INIT_WINDOW:
    {
    }
    break;
    case APP_CMD_TERM_WINDOW:
    {
    }
    break;
    case APP_CMD_WINDOW_RESIZED:
    {
      xiiInt32 width  = ANativeWindow_getWidth(pAndroidApp->window);
      xiiInt32 height = ANativeWindow_getHeight(pAndroidApp->window);

      if (width < 0)
        width = 0;

      if (height < 0)
        height = 0;

      xiiSizeU32 size((xiiUInt32)width, (xiiUInt32)height);
      pWindow->OnResize(size);
    }
    break;
    case APP_CMD_CONFIG_CHANGED:
    {
    }
    break;
    case APP_CMD_CONTENT_RECT_CHANGED:
    {
    }
    break;
    case APP_CMD_GAINED_FOCUS:
    {
      pWindow->OnFocus(true);
    }
    break;
    case APP_CMD_LOST_FOCUS:
    {
      pWindow->OnFocus(false);
    }
    break;

    default:
      break;
  }
}

xiiInt32 xiiWindow::InputEventCallback(android_app* pAndroidApp, AInputEvent* pInputEvent)
{
  xiiWindow* pWindow = reinterpret_cast<xiiWindow*>(pAndroidApp->userData);
  if (pWindow == nullptr && !pWindow->IsInitialized())
    return 0;

  if (pWindow->GetInputDevice())
    pWindow->GetInputDevice()->InputEventMessage(pAndroidApp, pInputEvent);

  xiiInt32 iEventType = AInputEvent_getType(pInputEvent);
  switch (iEventType)
  {
    case AINPUT_EVENT_TYPE_KEY:
    {
    }
    break;
    case AINPUT_EVENT_TYPE_MOTION:
    {
    }
    break;
    case AINPUT_EVENT_TYPE_FOCUS:
    {
    }
    break;
    case AINPUT_EVENT_TYPE_CAPTURE:
    {
    }
    break;
    case AINPUT_EVENT_TYPE_DRAG:
    {
    }
    break;
    case AINPUT_EVENT_TYPE_TOUCH_MODE:
    {
    }
    break;

    default:
      break;
  }

  return 0;
}

xiiWindowHandle xiiWindow::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}
