/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/ActorSystem/ActorPluginWindow.h>

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiActorPluginWindow, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiActorPluginWindow::Update()
{
  if (GetWindow())
  {
    GetWindow()->ProcessWindowMessages();
  }
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiActorPluginWindowOwner, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiActorPluginWindowOwner::~xiiActorPluginWindowOwner()
{
  // The window output target has a dependency to the window, e.g. the swapchain renders to it.
  // Explicitly destroy it first to ensure correct destruction order.
  m_pWindowOutputTarget.Clear();
  m_pWindow.Clear();
}

xiiWindowBase* xiiActorPluginWindowOwner::GetWindow() const
{
  return m_pWindow.Borrow();
}

xiiWindowOutputTargetBase* xiiActorPluginWindowOwner::GetOutputTarget() const
{
  return m_pWindowOutputTarget.Borrow();
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiActorPluginWindowShared, 1, xiiRTTINoAllocator)
  ;
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiWindowBase* xiiActorPluginWindowShared::GetWindow() const
{
  return m_pWindow;
}

xiiWindowOutputTargetBase* xiiActorPluginWindowShared::GetOutputTarget() const
{
  return m_pWindowOutputTarget;
}

XII_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_ActorPluginWindow);
