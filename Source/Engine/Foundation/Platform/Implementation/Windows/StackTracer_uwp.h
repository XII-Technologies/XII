#ifdef XII_STACKTRACER_UWP_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define XII_STACKTRACER_UWP_INL_H_INCLUDED

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

void xiiStackTracer::OnPluginEvent(const xiiPluginEvent& e) {}

// static
xiiUInt32 xiiStackTracer::GetStackTrace(xiiArrayPtr<void*>& trace, void* pContext)
{
  return 0;
}

// static
void xiiStackTracer::ResolveStackTrace(const xiiArrayPtr<void*>& trace, PrintFunc printFunc)
{
  char szBuffer[512] = "Stack Traces are currently not supported on UWP";

  printFunc(szBuffer);
}
