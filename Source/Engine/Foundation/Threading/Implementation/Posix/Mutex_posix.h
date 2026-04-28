/// Copyright (c) Theophilus Eriata. All Rights Reserved.

XII_ALWAYS_INLINE xiiMutex::xiiMutex()
{
  pthread_mutexattr_t mutexAttributes;
  pthread_mutexattr_init(&mutexAttributes);
  pthread_mutexattr_settype(&mutexAttributes, PTHREAD_MUTEX_RECURSIVE);

  pthread_mutex_init(&m_hHandle, &mutexAttributes);

  pthread_mutexattr_destroy(&mutexAttributes);
}

XII_ALWAYS_INLINE xiiMutex::~xiiMutex()
{
  pthread_mutex_destroy(&m_hHandle);
}

XII_ALWAYS_INLINE void xiiMutex::Lock()
{
  pthread_mutex_lock(&m_hHandle);
  ++m_iLockCount;
}

XII_ALWAYS_INLINE xiiResult xiiMutex::TryLock()
{
  if (pthread_mutex_trylock(&m_hHandle) == 0)
  {
    ++m_iLockCount;
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}
XII_ALWAYS_INLINE void xiiMutex::Unlock()
{
  --m_iLockCount;
  pthread_mutex_unlock(&m_hHandle);
}
