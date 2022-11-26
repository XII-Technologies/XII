#pragma once

// Deactivate Doxygen document generation for the following block.
/// \cond

#include <pthread.h>
#include <semaphore.h>

using xiiThreadHandle       = pthread_t;
using xiiThreadID           = pthread_t;
using xiiMutexHandle        = pthread_mutex_t;
using xiiOSThreadEntryPoint = void* (*)(void* pThreadParameter);

struct xiiSemaphoreHandle
{
  sem_t* m_pNamedOrUnnamed = nullptr;
  sem_t* m_pNamed          = nullptr;
  sem_t  m_Unnamed;
};

#define XII_THREAD_CLASS_ENTRY_POINT void* xiiThreadClassEntryPoint(void* pThreadParameter);

struct xiiConditionVariableData
{
  pthread_cond_t m_ConditionVariable;
};


/// \endcond
