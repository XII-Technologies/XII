/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

// Posix implementation of thread helper functions

#include <pthread.h>

static pthread_t g_MainThread = (pthread_t)0;

void xiiThreadUtils::Initialize()
{
  g_MainThread = pthread_self();
}

void xiiThreadUtils::YieldTimeSlice()
{
  sched_yield();
}

void xiiThreadUtils::YieldHardwareThread()
{
  // No equivalent to mm_pause on linux
}

void xiiThreadUtils::Sleep(const xiiTime& duration)
{
  timespec SleepTime;
  SleepTime.tv_sec  = duration.GetSeconds();
  SleepTime.tv_nsec = ((xiiInt64)duration.GetMilliseconds() * 1000000LL) % 1000000000LL;
  nanosleep(&SleepTime, nullptr);
}

// xiiThreadHandle xiiThreadUtils::GetCurrentThreadHandle()
//{
//  return pthread_self();
//}

xiiThreadID xiiThreadUtils::GetCurrentThreadID()
{
  return pthread_self();
}

bool xiiThreadUtils::IsMainThread()
{
  return pthread_self() == g_MainThread;
}
