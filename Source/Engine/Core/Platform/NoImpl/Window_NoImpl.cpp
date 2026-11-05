/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#if XII_DISABLED(XII_SUPPORTS_SDL)

#  include <Core/System/Window.h>

xiiResult xiiWindow::Initialize()
{
  XII_ASSERT_NOT_IMPLEMENTED;
  return XII_FAILURE;
}

xiiResult xiiWindow::Destroy()
{
  XII_ASSERT_NOT_IMPLEMENTED;
  return XII_FAILURE;
}

xiiResult xiiWindow::Resize(const xiiSizeU32& newWindowSize)
{
  XII_IGNORE_UNUSED(newWindowSize);
  XII_ASSERT_NOT_IMPLEMENTED;
  return XII_FAILURE;
}

xiiResult xiiWindow::SetTitle(xiiStringView sTitle)
{
  XII_IGNORE_UNUSED(sTitle);
  return XII_FAILURE;
}

xiiResult xiiWindow::ApplyWindowSettings(const xiiWindowCreationDescription& description)
{
  XII_IGNORE_UNUSED(description);
  return XII_FAILURE;
}

void xiiWindow::ProcessWindowMessages()
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

xiiWindowHandle xiiWindow::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}

#endif
