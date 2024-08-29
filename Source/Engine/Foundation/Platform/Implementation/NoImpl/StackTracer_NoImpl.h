#include <Foundation/FoundationInternal.h>

XII_FOUNDATION_INTERNAL_HEADER

void xiiStackTracer::OnPluginEvent(const xiiPluginEvent& e) {}

xiiUInt32 xiiStackTracer::GetStackTrace(xiiArrayPtr<void*>& trace, void* pContext)
{
  return 0U;
}

void xiiStackTracer::ResolveStackTrace(const xiiArrayPtr<void*>& trace, PrintFunc printFunc)
{
}
