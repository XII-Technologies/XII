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
  XII_ASSERT_NOT_IMPLEMENTED;
  return XII_FAILURE;
}

void xiiWindow::ProcessWindowMessages()
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

void xiiWindow::OnResize(const xiiSizeU32& newWindowSize)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

xiiWindowHandle xiiWindow::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}

#endif
