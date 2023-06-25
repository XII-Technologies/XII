#include <Core/CorePCH.h>

#include <Core/System/Window.h>
#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

static LRESULT CALLBACK xiiWindowsMessageFuncTrampoline(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  xiiWindow* pWindow = reinterpret_cast<xiiWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

  if (pWindow != nullptr && pWindow->IsInitialized())
  {
    if (pWindow->GetInputDevice())
      pWindow->GetInputDevice()->WindowMessage(xiiMinWindows::FromNative(hWnd), msg, wparam, lparam);

    switch (msg)
    {
      case WM_CLOSE:
        pWindow->OnClickClose();
        return 0;

      case WM_SETFOCUS:
        pWindow->OnFocus(true);
        return 0;

      case WM_KILLFOCUS:
        pWindow->OnFocus(false);
        return 0;

      case WM_SIZE:
      {
        xiiSizeU32 size(LOWORD(lparam), HIWORD(lparam));
        pWindow->OnResize(size);
      }
      break;

      case WM_SYSKEYDOWN:
      {
        // Filter this message out, otherwise pressing ALT will give focus to the system menu, locking out other actions
        // until ALT is pressed again, which is typically not desired
        return 0;
      }

      case WM_MOVE:
      {
        pWindow->OnWindowMove((xiiInt32)(xiiInt16)LOWORD(lparam), (xiiInt32)(xiiInt16)HIWORD(lparam));
      }
      break;
    }

    pWindow->OnWindowMessage(xiiMinWindows::FromNative(hWnd), msg, wparam, lparam);
  }

  return DefWindowProcW(hWnd, msg, wparam, lparam);
}

xiiResult xiiWindow::Initialize()
{
  XII_LOG_BLOCK("xiiWindow::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
  {
    Destroy().IgnoreResult();
  }

  XII_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");

  // Initialize window class
  WNDCLASSEXW windowClass   = {};
  windowClass.cbSize        = sizeof(WNDCLASSEXW);
  windowClass.style         = CS_HREDRAW | CS_VREDRAW;
  windowClass.hInstance     = GetModuleHandleW(nullptr);
  windowClass.hIcon         = LoadIcon(GetModuleHandleW(nullptr), MAKEINTRESOURCE(101)); /// \todo Expose icon functionality somehow (101 == IDI_ICON1, see resource.h)
  windowClass.hCursor       = LoadCursor(nullptr, IDC_ARROW);
  windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  windowClass.lpszClassName = L"xiiWin32Window";
  windowClass.lpfnWndProc   = xiiWindowsMessageFuncTrampoline;

  if (!RegisterClassExW(&windowClass)) /// \todo Test & support for multiple windows.
  {
    DWORD error = GetLastError();

    if (error != ERROR_CLASS_ALREADY_EXISTS)
    {
      xiiLog::Error("Failed to create xiiWindow window class! (error code '{0}')", xiiArgU(error));
      return XII_FAILURE;
    }
  }

  // Setup fullscreen mode
  if (m_CreationDescription.m_WindowMode == xiiWindowMode::FullscreenFixedResolution)
  {
    xiiLog::Dev("Changing display resolution for fullscreen mode to {0}*{1}", m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height);

    DEVMODEW dmScreenSettings     = {};
    dmScreenSettings.dmSize       = sizeof(DEVMODEW);
    dmScreenSettings.dmPelsWidth  = m_CreationDescription.m_Resolution.width;
    dmScreenSettings.dmPelsHeight = m_CreationDescription.m_Resolution.height;
    dmScreenSettings.dmBitsPerPel = 32;
    dmScreenSettings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

    if (ChangeDisplaySettingsW(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
    {
      m_CreationDescription.m_WindowMode = xiiWindowMode::FullscreenBorderlessNativeResolution;
      XII_SUCCEED_OR_RETURN(m_CreationDescription.AdjustWindowSizeAndPosition());

      xiiLog::Error("Failed to change display resolution for fullscreen window. Falling back to borderless window.");
    }
  }

  // Setup window style
  DWORD dwExStyle     = WS_EX_APPWINDOW;
  DWORD dwWindowStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

  if (m_CreationDescription.m_WindowMode == xiiWindowMode::WindowFixedResolution || m_CreationDescription.m_WindowMode == xiiWindowMode::WindowResizable)
  {
    xiiLog::Dev("Window is not fullscreen.");
    dwWindowStyle |= WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
  }
  else
  {
    xiiLog::Dev("Window is fullscreen.");
    dwWindowStyle |= WS_POPUP;
  }

  if (m_CreationDescription.m_WindowMode == xiiWindowMode::WindowResizable)
  {
    xiiLog::Dev("Window is resizable.");
    dwWindowStyle |= WS_MAXIMIZEBOX | WS_THICKFRAME;
  }


  // Create rectangle for window
  RECT Rect = {0, 0, (LONG)m_CreationDescription.m_Resolution.width, (LONG)m_CreationDescription.m_Resolution.height};

  // Account for left or top placed task bars
  if (m_CreationDescription.m_WindowMode == xiiWindowMode::WindowFixedResolution || m_CreationDescription.m_WindowMode == xiiWindowMode::WindowResizable)
  {
    // Adjust for borders and bars etc.
    AdjustWindowRectEx(&Rect, dwWindowStyle, FALSE, dwExStyle);

    // Top left position now may be negative (due to AdjustWindowRectEx)
    // Move
    Rect.right -= Rect.left;
    Rect.bottom -= Rect.top;
    // Apply user translation
    Rect.left = m_CreationDescription.m_Position.x;
    Rect.top  = m_CreationDescription.m_Position.y;
    Rect.right += m_CreationDescription.m_Position.x;
    Rect.bottom += m_CreationDescription.m_Position.y;

    // Move into work area
    RECT RectWorkArea = {0};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &RectWorkArea, 0);

    Rect.left += RectWorkArea.left;
    Rect.right += RectWorkArea.left;
    Rect.top += RectWorkArea.top;
    Rect.bottom += RectWorkArea.top;
  }

  const xiiInt32 iWidth  = Rect.right - Rect.left;
  const xiiInt32 iHeight = Rect.bottom - Rect.top;

  xiiLog::Info("Window Dimensions: {0}*{1} at left/top origin ({2}, {3}).", iWidth, iHeight, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y);

  // Create window
  xiiStringWChar sTitelWChar(m_CreationDescription.m_Title.GetData());
  const wchar_t* sTitelWCharRaw = sTitelWChar.GetData();
  m_hWindowHandle               = xiiMinWindows::FromNative(CreateWindowExW(dwExStyle, windowClass.lpszClassName, sTitelWCharRaw, dwWindowStyle, m_CreationDescription.m_Position.x, m_CreationDescription.m_Position.y, iWidth, iHeight, nullptr, nullptr, windowClass.hInstance, nullptr));

  if (m_hWindowHandle == INVALID_HANDLE_VALUE)
  {
    xiiLog::Error("Failed to create window.");
    return XII_FAILURE;
  }

  auto windowHandle = xiiMinWindows::ToNative(m_hWindowHandle);

  // Safe window pointer for lookup in xiiWindowsMessageFuncTrampoline
  SetWindowLongPtrW(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  // Show window and activate if required
  ShowWindow(windowHandle, m_CreationDescription.m_bSetForegroundOnInit ? SW_SHOWNORMAL : SW_SHOWNOACTIVATE);
  if (m_CreationDescription.m_bSetForegroundOnInit)
  {
    SetActiveWindow(windowHandle);
    SetFocus(windowHandle);
    SetForegroundWindow(windowHandle);
  }

  RECT r;
  GetClientRect(windowHandle, &r);

  // Force size change to the desired size if CreateWindowExW 'fixed' the size to fit into your current monitor.
  if (m_CreationDescription.m_WindowMode == xiiWindowMode::WindowFixedResolution && (m_CreationDescription.m_Resolution.width != r.right - r.left || m_CreationDescription.m_Resolution.height != r.bottom - r.top))
  {
    ::SetWindowPos(windowHandle, HWND_NOTOPMOST, 0, 0, iWidth, iHeight, SWP_NOSENDCHANGING | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
    GetClientRect(windowHandle, &r);
  }

  m_CreationDescription.m_Resolution.width  = r.right - r.left;
  m_CreationDescription.m_Resolution.height = r.bottom - r.top;

  m_bInitialized = true;
  xiiLog::Success("Created window successfully. Resolution is {0}*{1}", GetClientAreaSize().width, GetClientAreaSize().height);

  m_pInputDevice = XII_DEFAULT_NEW(xiiStandardInputDevice, m_CreationDescription.m_uiWindowNumber);
  m_pInputDevice->SetClipMouseCursor(m_CreationDescription.m_bClipMouseCursor ? xiiMouseCursorClipMode::ClipToWindowImmediate : xiiMouseCursorClipMode::NoClip);
  m_pInputDevice->SetShowMouseCursor(m_CreationDescription.m_bShowMouseCursor);

  return XII_SUCCESS;
}

xiiResult xiiWindow::Destroy()
{
  if (!m_bInitialized)
    return XII_SUCCESS;

  if (GetInputDevice() && GetInputDevice()->GetClipMouseCursor() != xiiMouseCursorClipMode::NoClip)
  {
    GetInputDevice()->SetClipMouseCursor(xiiMouseCursorClipMode::NoClip);
  }

  XII_LOG_BLOCK("xiiWindow::Destroy");

  xiiResult Res = XII_SUCCESS;

  m_pInputDevice = nullptr;

  if (m_CreationDescription.m_WindowMode == xiiWindowMode::FullscreenFixedResolution)
    ChangeDisplaySettingsW(nullptr, 0);

  HWND hWindow = xiiMinWindows::ToNative(GetNativeWindowHandle());
  // The following line of code is a work around, because 'LONG_PTR pNull = reinterpret_cast<LONG_PTR>(nullptr)' crashes the VS 2010 32 Bit
  // compiler :-(
  LONG_PTR pNull = 0;
  // Set the window ptr to null before calling DestroyWindow as it might trigger callbacks and we are potentially already in the destructor, making any virtual function call unsafe.
  SetWindowLongPtrW(hWindow, GWLP_USERDATA, pNull);

  if (!DestroyWindow(hWindow))
  {
    xiiLog::SeriousWarning("DestroyWindow failed.");
    Res = XII_FAILURE;
  }

  // Actually nobody cares about this, all Window Classes are cleared when the application closes
  // in the mean time, having multiple windows will just result in errors when one is closed,
  // as the Window Class must not be in use anymore when one calls UnregisterClassW
#if 0
  if (!UnregisterClassW(L"xiiWin32Window", GetModuleHandleW(nullptr)))
  {
    xiiLog::SeriousWarning("UnregisterClassW failed.");
    Res = XII_FAILURE;
  }
#endif

  m_bInitialized  = false;
  m_hWindowHandle = INVALID_WINDOW_HANDLE_VALUE;

  if (Res == XII_SUCCESS)
    xiiLog::Success("Window destroyed.");
  else
    xiiLog::SeriousWarning("There were problems to destroy the window properly.");

  return Res;
}

xiiResult xiiWindow::Resize(const xiiSizeU32& newWindowSize)
{
  auto windowHandle = xiiMinWindows::ToNative(m_hWindowHandle);
  BOOL res          = ::SetWindowPos(windowHandle, HWND_NOTOPMOST, 0, 0, newWindowSize.width, newWindowSize.height, SWP_NOSENDCHANGING | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
  return res != FALSE ? XII_SUCCESS : XII_FAILURE;
}

void xiiWindow::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return;

  MSG msg = {0};
  while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
  {
    if (msg.message == WM_QUIT)
    {
      Destroy().IgnoreResult();
      return;
    }

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
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
