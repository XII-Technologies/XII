
xiiEditorProcessViewWindow::~xiiEditorProcessViewWindow()
{
  xiiGALDevice::GetDefaultDevice()->WaitIdle();

  XII_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call xiiGALDevice::WaitIdle before destroying a window.");
}

xiiResult xiiEditorProcessViewWindow::UpdateWindow(xiiWindowHandle hParentWindow, xiiUInt16 uiWidth, xiiUInt16 uiHeight)
{
  m_hWnd     = hParentWindow;
  m_uiWidth  = uiWidth;
  m_uiHeight = uiHeight;

  return XII_SUCCESS;
}
