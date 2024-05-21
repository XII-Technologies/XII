#include <Core/CorePCH.h>

#include <Core/GameState/GameStateWindow.h>

xiiGameStateWindow::xiiGameStateWindow(const xiiWindowCreationDesc& windowdesc, xiiDelegate<void()> onClickClose) :
  m_OnClickClose(onClickClose)
{
  m_CreationDescription = windowdesc;
  m_CreationDescription.AdjustWindowSizeAndPosition().IgnoreResult();

  Initialize().IgnoreResult();
}

xiiGameStateWindow::~xiiGameStateWindow()
{
  Destroy().IgnoreResult();
}

void xiiGameStateWindow::ResetOnClickClose(xiiDelegate<void()> onClickClose)
{
  m_OnClickClose = onClickClose;
}

void xiiGameStateWindow::OnClickClose()
{
  if (m_OnClickClose.IsValid())
  {
    m_OnClickClose();
  }
}

void xiiGameStateWindow::OnResize(const xiiSizeU32& newWindowSize)
{
  xiiLog::Info("Resolution changed to {0} * {1}", newWindowSize.width, newWindowSize.height);

  m_CreationDescription.m_Resolution = newWindowSize;
}

XII_STATICLINK_FILE(Core, Core_GameState_Implementation_GameStateWindow);
