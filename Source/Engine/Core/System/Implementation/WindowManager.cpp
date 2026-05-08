#include <Core/CorePCH.h>

#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <Core/System/Window.h>
#include <Core/System/WindowManager.h>
#include <Foundation/Configuration/Startup.h>

XII_IMPLEMENT_SINGLETON(xiiWindowManager);

//////////////////////////////////////////////////////////////////////////

static xiiUniquePtr<xiiWindowManager> s_pWindowManager;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Core, xiiWindowManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    s_pWindowManager = XII_DEFAULT_NEW(xiiWindowManager);
  }
  ON_CORESYSTEMS_SHUTDOWN
  {
    s_pWindowManager.Clear();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (s_pWindowManager)
    {
      s_pWindowManager->CloseAll(nullptr);
    }
  }

XII_END_SUBSYSTEM_DECLARATION;

// clang-format on

//////////////////////////////////////////////////////////////////////////

xiiWindowManager::xiiWindowManager() :
  m_SingletonRegistrar(this)
{
}

xiiWindowManager::~xiiWindowManager()
{
  CloseAll(nullptr);
}

void xiiWindowManager::Update()
{
  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->m_pWindow->ProcessWindowMessages();
  }
}

void xiiWindowManager::Close(xiiRegisteredWindowHandle hWindow)
{
  xiiUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(hWindow.GetInternalID(), pDataPtr))
    return;

  Data* pData = pDataPtr->Borrow();
  XII_ASSERT_DEV(pData != nullptr, "Invalid window data");

  if (pData->m_OnDestroy.IsValid())
  {
    pData->m_OnDestroy(hWindow);
  }

  // The window output target has a dependency to the window, e.g. the swap chain renders to it.
  // Explicitly destroy it first to ensure correct destruction order.

  if (pData->m_pOutputTarget)
  {
    pData->m_pOutputTarget.Clear();
  }

  if (pData->m_pWindow)
  {
    pData->m_pWindow.Clear();
  }

  m_Data.Remove(hWindow.GetInternalID());
}

void xiiWindowManager::CloseAll(const void* pCreatedBy)
{
  xiiTemporaryArray<xiiRegisteredWindowHandle> toClose;

  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    if (pCreatedBy == nullptr || it.Value()->m_pCreatedBy == pCreatedBy)
    {
      toClose.PushBack(xiiRegisteredWindowHandle(it.Id()));
    }
  }

  for (const xiiRegisteredWindowHandle& hWindow : toClose)
  {
    Close(hWindow);
  }
}

bool xiiWindowManager::IsValid(xiiRegisteredWindowHandle hWindow) const
{
  return m_Data.Contains(hWindow.GetInternalID());
}

void xiiWindowManager::GetRegistered(xiiDynamicArray<xiiRegisteredWindowHandle>& out_windowHandles, const void* pCreatedBy /*= nullptr*/)
{
  out_windowHandles.Clear();

  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    if (pCreatedBy == nullptr || it.Value()->m_pCreatedBy == pCreatedBy)
    {
      out_windowHandles.PushBack(xiiRegisteredWindowHandle(it.Id()));
    }
  }
}

xiiRegisteredWindowHandle xiiWindowManager::Register(xiiStringView sName, const void* pCreatedBy, xiiUniquePtr<xiiWindowBase>&& pWindow)
{
  XII_ASSERT_ALWAYS(pCreatedBy != nullptr, "pCreatedBy is invalid.");
  XII_ASSERT_ALWAYS(pWindow != nullptr, "pWindow is invalid.");

  xiiUniquePtr<Data> pData = XII_DEFAULT_NEW(Data);
  pData->m_sName           = sName;
  pData->m_pCreatedBy      = pCreatedBy;
  pData->m_pWindow         = std::move(pWindow);

  return xiiRegisteredWindowHandle(m_Data.Insert(std::move(pData)));
}

void xiiWindowManager::SetOutputTarget(xiiRegisteredWindowHandle hWindow, xiiUniquePtr<xiiWindowOutputTargetBase>&& pOutputTarget)
{
  xiiUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(hWindow.GetInternalID(), pDataPtr))
    return;

  (*pDataPtr)->m_pOutputTarget = std::move(pOutputTarget);
}

void xiiWindowManager::SetDestroyCallback(xiiRegisteredWindowHandle hWindow, xiiWindowDestroyFunc onDestroyCallback)
{
  xiiUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(hWindow.GetInternalID(), pDataPtr))
    return;

  (*pDataPtr)->m_OnDestroy = onDestroyCallback;
}

xiiStringView xiiWindowManager::GetName(xiiRegisteredWindowHandle hWindow) const
{
  if (!m_Data.Contains(hWindow.GetInternalID()))
    return xiiStringView();

  return m_Data[hWindow.GetInternalID()]->m_sName;
}

xiiWindowBase* xiiWindowManager::GetWindow(xiiRegisteredWindowHandle hWindow) const
{
  if (!m_Data.Contains(hWindow.GetInternalID()))
    return nullptr;

  return m_Data[hWindow.GetInternalID()]->m_pWindow.Borrow();
}

xiiWindowOutputTargetBase* xiiWindowManager::GetOutputTarget(xiiRegisteredWindowHandle hWindow) const
{
  if (!m_Data.Contains(hWindow.GetInternalID()))
    return nullptr;

  return m_Data[hWindow.GetInternalID()]->m_pOutputTarget.Borrow();
}

XII_STATICLINK_FILE(Core, Core_System_Implementation_WindowManager);
