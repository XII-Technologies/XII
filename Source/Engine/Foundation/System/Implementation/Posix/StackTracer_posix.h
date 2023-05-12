#ifdef XII_STACKTRACER_POSIX_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define XII_STACKTRACER_POSIX_INL_H_INCLUDED

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Math/Math.h>
#include <execinfo.h>

void xiiStackTracer::OnPluginEvent(const xiiPluginEvent& e)
{}

// static
xiiUInt32 xiiStackTracer::GetStackTrace(xiiArrayPtr<void*>& trace, void* pContext)
{
  xiiInt32 iSymbols = backtrace(trace.GetPtr(), trace.GetCount());

  return iSymbols;
}

// static
void xiiStackTracer::ResolveStackTrace(const xiiArrayPtr<void*>& trace, PrintFunc printFunc)
{
  char szBuffer[512];

  char** ppSymbols = backtrace_symbols(trace.GetPtr(), trace.GetCount());

  if (ppSymbols != nullptr)
  {
    for (xiiUInt32 i = 0; i < trace.GetCount(); i++)
    {
      xiiInt32 iLen = xiiMath::Min(strlen(ppSymbols[i]), (size_t)XII_ARRAY_SIZE(szBuffer) - 2);
      memcpy(szBuffer, ppSymbols[i], iLen);
      szBuffer[iLen]     = '\n';
      szBuffer[iLen + 1] = '\0';

      printFunc(szBuffer);
    }

    free(ppSymbols);
  }
}
