#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER
#include <time.h>

void xiiTime::Initialize()
{}

xiiTime xiiTime::Now()
{
  struct timespec sp;
  clock_gettime(CLOCK_MONOTONIC_RAW, &sp);

  return xiiTime::Seconds((double)sp.tv_sec + (double)(sp.tv_nsec / 1000000000.0));
}
