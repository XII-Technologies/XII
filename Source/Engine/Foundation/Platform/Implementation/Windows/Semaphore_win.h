#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Basics/Platform/Win/MinWindows.h>
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

  LPSECURITY_ATTRIBUTES secAttr = nullptr; // default
  const DWORD           flags   = 0;       // reserved but unused
  const DWORD           access  = STANDARD_RIGHTS_ALL | SEMAPHORE_MODIFY_STATE /* needed for ReleaseSemaphore */;

  if (sSharedName.IsEmpty())
  {
    // create an unnamed semaphore

    m_hSemaphore = CreateSemaphoreExW(secAttr, uiInitialTokenCount, xiiMath::MaxValue<xiiInt32>(), nullptr, flags, access);
  }
  else
  {
    // create a named semaphore in the 'Local' namespace
    // these are visible session wide, ie. all processes by the same user account can see these, but not across users

    const xiiStringBuilder semaphoreName("Local\\", sSharedName);

    m_hSemaphore = CreateSemaphoreExW(secAttr, uiInitialTokenCount, xiiMath::MaxValue<xiiInt32>(), xiiStringWChar(semaphoreName).GetData(), flags, access);
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

  const DWORD access         = SYNCHRONIZE /* needed for WaitForSingleObject */ | SEMAPHORE_MODIFY_STATE /* needed for ReleaseSemaphore */;
  const BOOL  inheriteHandle = FALSE;

  XII_ASSERT_DEV(!sSharedName.IsEmpty(), "Name of semaphore to open mustn't be empty.");

  const xiiStringBuilder semaphoreName("Local\\", sSharedName);

  m_hSemaphore = OpenSemaphoreW(access, inheriteHandle, xiiStringWChar(semaphoreName).GetData());

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
