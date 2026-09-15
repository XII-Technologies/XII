/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#include <Foundation/Basics/Platform/Windows/MinWindows.h>
#include <Foundation/Strings/StringBuilder.h>

xiiSemaphore::xiiSemaphore()
{
  m_hSemaphore = nullptr;
}

xiiSemaphore::~xiiSemaphore()
{
  if (m_hSemaphore != nullptr)
  {
    CloseHandle(m_hSemaphore);
    m_hSemaphore = nullptr;
  }
}

xiiResult xiiSemaphore::Create(xiiUInt32 uiInitialTokenCount, xiiStringView sSharedName /*= nullptr*/)
{
  XII_ASSERT_DEV(m_hSemaphore == nullptr, "Semaphore can't be recreated.");

  LPSECURITY_ATTRIBUTES pSecurityAttributes = nullptr; // Default.
  const DWORD           uiFlags             = 0;       // Reserved but unused.
  const DWORD           uiAccess            = STANDARD_RIGHTS_ALL | SEMAPHORE_MODIFY_STATE /* needed for ReleaseSemaphore */;

  if (sSharedName.IsEmpty())
  {
    // Create an unnamed semaphore.

    m_hSemaphore = CreateSemaphoreExW(pSecurityAttributes, uiInitialTokenCount, xiiMath::MaxValue<xiiInt32>(), nullptr, uiFlags, uiAccess);
  }
  else
  {
    // Create a named semaphore in the 'Local' namespace.
    // These are visible session wide, ie. all processes by the same user account can see these, but not across users.

    const xiiStringBuilder sSemaphoreName("Local\\", sSharedName);

    m_hSemaphore = CreateSemaphoreExW(pSecurityAttributes, uiInitialTokenCount, xiiMath::MaxValue<xiiInt32>(), xiiStringWChar(sSemaphoreName).GetData(), uiFlags, uiAccess);
  }

  if (m_hSemaphore == nullptr)
  {
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiSemaphore::Open(xiiStringView sSharedName)
{
  XII_ASSERT_DEV(m_hSemaphore == nullptr, "Semaphore can't be recreated.");

  const DWORD uiAccess         = SYNCHRONIZE /* Required for WaitForSingleObject */ | SEMAPHORE_MODIFY_STATE /* Required for ReleaseSemaphore */;
  const BOOL  uiInheriteHandle = FALSE;

  XII_ASSERT_DEV(!sSharedName.IsEmpty(), "Name of semaphore to open mustn't be empty.");

  const xiiStringBuilder sSemaphoreName("Local\\", sSharedName);

  m_hSemaphore = OpenSemaphoreW(uiAccess, uiInheriteHandle, xiiStringWChar(sSemaphoreName).GetData());

  if (m_hSemaphore == nullptr)
  {
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiSemaphore::AcquireToken()
{
  XII_ASSERT_DEV(m_hSemaphore != nullptr, "Invalid semaphore.");
  XII_VERIFY(WaitForSingleObject(m_hSemaphore, INFINITE) == WAIT_OBJECT_0, "Semaphore token acquisition failed.");
}

void xiiSemaphore::ReturnToken()
{
  XII_ASSERT_DEV(m_hSemaphore != nullptr, "Invalid semaphore.");
  XII_VERIFY(ReleaseSemaphore(m_hSemaphore, 1, nullptr) != 0, "Returning a semaphore token failed, most likely due to a AcquireToken() / ReturnToken() mismatch.");
}

xiiResult xiiSemaphore::TryAcquireToken(xiiTime timeout)
{
  XII_ASSERT_DEV(m_hSemaphore != nullptr, "Invalid semaphore.");

  const xiiUInt32 uiResult = WaitForSingleObject(m_hSemaphore, xiiMath::FloatToInt(timeout.AsFloatInSeconds()));

  if (uiResult == WAIT_OBJECT_0)
  {
    return XII_SUCCESS;
  }

  XII_ASSERT_DEV(uiResult == WAIT_OBJECT_0 || uiResult == WAIT_TIMEOUT, "Semaphore TryAcquireToken (WaitForSingleObject) failed with error code {}.", uiResult);

  return XII_FAILURE;
}
