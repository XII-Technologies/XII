/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Threading/Semaphore.h>

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <cerrno>

#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

XII_WARNING_PUSH()
// On OSX sem_destroy and sem_init are deprecated
XII_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")

xiiSemaphore::xiiSemaphore() = default;

xiiSemaphore::~xiiSemaphore()
{
  if (m_hSemaphore.m_pNamedOrUnnamed != nullptr)
  {
    if (m_hSemaphore.m_pNamed != nullptr)
    {
      sem_close(m_hSemaphore.m_pNamed);
    }
    else
    {
      sem_destroy(&m_hSemaphore.m_Unnamed);
    }
  }
}

xiiResult xiiSemaphore::Create(xiiUInt32 uiInitialTokenCount, xiiStringView sSharedName /*= nullptr*/)
{
  if (sSharedName.IsEmpty())
  {
#if XII_ENABLED(XII_PLATFORM_OSX)
    // Darwin does not implement unnamed POSIX semaphores. Unlink a uniquely
    // named semaphore immediately so its lifetime is owned by this handle.
    static xiiAtomicInteger32 s_iSemaphoreId;
    char                      name[32];
    snprintf(name, XII_ARRAY_SIZE(name), "/xii-%u-%u", static_cast<unsigned>(getpid()), static_cast<unsigned>(s_iSemaphoreId.Increment()));
    m_hSemaphore.m_pNamed = sem_open(name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, uiInitialTokenCount);
    if (m_hSemaphore.m_pNamed == SEM_FAILED)
    {
      m_hSemaphore.m_pNamed = nullptr;
      return XII_FAILURE;
    }
    sem_unlink(name);
    m_hSemaphore.m_pNamedOrUnnamed = m_hSemaphore.m_pNamed;
#else
    // create an unnamed semaphore

    if (sem_init(&m_hSemaphore.m_Unnamed, 0, uiInitialTokenCount) != 0)
    {
      return XII_FAILURE;
    }

    m_hSemaphore.m_pNamedOrUnnamed = &m_hSemaphore.m_Unnamed;
#endif
  }
  else
  {
    // create a named semaphore

    // documentation is unclear about access rights, just throwing everything at it for good measure
    xiiStringBuilder tmp;
    m_hSemaphore.m_pNamed = sem_open(sSharedName.GetData(tmp), O_CREAT | O_EXCL, S_IRWXU | S_IRWXO | S_IRWXG, uiInitialTokenCount);

    if (m_hSemaphore.m_pNamed == SEM_FAILED)
    {
      m_hSemaphore.m_pNamed = nullptr;
      return XII_FAILURE;
    }

    m_hSemaphore.m_pNamedOrUnnamed = m_hSemaphore.m_pNamed;
  }

  return XII_SUCCESS;
}

xiiResult xiiSemaphore::Open(xiiStringView sSharedName)
{
  XII_ASSERT_DEV(!sSharedName.IsEmpty(), "Name of semaphore to open mustn't be empty.");

  // open a named semaphore

  xiiStringBuilder tmp;
  m_hSemaphore.m_pNamed = sem_open(sSharedName.GetData(tmp), 0);

  if (m_hSemaphore.m_pNamed == SEM_FAILED)
  {
    m_hSemaphore.m_pNamed = nullptr;
    return XII_FAILURE;
  }

  m_hSemaphore.m_pNamedOrUnnamed = m_hSemaphore.m_pNamed;

  return XII_SUCCESS;
}

void xiiSemaphore::AcquireToken()
{
  XII_VERIFY(sem_wait(m_hSemaphore.m_pNamedOrUnnamed) == 0, "Semaphore token acquisition failed.");
}

void xiiSemaphore::ReturnToken()
{
  XII_VERIFY(sem_post(m_hSemaphore.m_pNamedOrUnnamed) == 0, "Returning a semaphore token failed, most likely due to a AcquireToken() / ReturnToken() mismatch.");
}

xiiResult xiiSemaphore::TryAcquireToken(xiiTime timeout)
{
#if XII_ENABLED(XII_PLATFORM_OSX)
  // Darwin has no sem_timedwait. Bound retries by the monotonic engine clock.
  const xiiTime deadline = xiiTime::Now() + timeout;
  do
  {
    if (sem_trywait(m_hSemaphore.m_pNamedOrUnnamed) == 0)
      return XII_SUCCESS;
    if (errno != EAGAIN && errno != EINTR)
      return XII_FAILURE;
    const xiiTime remaining = deadline - xiiTime::Now();
    if (remaining <= xiiTime::MakeZero())
      break;
    xiiThreadUtils::Sleep(xiiMath::Min(remaining, xiiTime::MakeFromMilliseconds(1)));
  } while (true);
  return XII_FAILURE;
#else
  // documentation is unclear whether one needs to check errno, or not
  // assuming that this will return 0 only when trywait got a token

  struct timespec spec = {};
  clock_gettime(CLOCK_REALTIME, &spec);
  spec.tv_sec += timeout.AsFloatInSeconds();

  if (sem_timedwait(m_hSemaphore.m_pNamedOrUnnamed, &spec) == 0)
    return XII_SUCCESS;

  return XII_FAILURE;
#endif
}

XII_WARNING_POP()
