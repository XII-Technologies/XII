/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/StackTracer.h>

static LONG WINAPI xiiCrashHandlerFunc(struct _EXCEPTION_POINTERS* pExceptionInfo)
{
  static xiiMutex s_CrashMutex;
  XII_LOCK(s_CrashMutex);

  static bool s_bAlreadyHandled = false;

  if (s_bAlreadyHandled == false)
  {
    if (xiiCrashHandler::GetCrashHandler() != nullptr)
    {
      s_bAlreadyHandled = true;
      xiiCrashHandler::GetCrashHandler()->HandleCrash(pExceptionInfo);
    }
  }

  return EXCEPTION_CONTINUE_SEARCH;
}

void xiiCrashHandler::SetCrashHandler(xiiCrashHandler* pHandler)
{
  s_pActiveHandler = pHandler;

  if (s_pActiveHandler != nullptr)
  {
    SetUnhandledExceptionFilter(xiiCrashHandlerFunc);
  }
  else
  {
    SetUnhandledExceptionFilter(nullptr);
  }
}

bool xiiCrashHandler_WriteMiniDump::WriteOwnProcessMiniDump(void* pOsSpecificData)
{
  xiiStatus res = xiiMiniDumpUtils::WriteOwnProcessMiniDump(m_sDumpFilePath, (_EXCEPTION_POINTERS*)pOsSpecificData);
  if (res.Failed())
  {
    xiiLog::Printf("WriteOwnProcessMiniDump failed: %s\n", res.GetMessageString().GetData());
  }
  return res.Succeeded();
}

void xiiCrashHandler_WriteMiniDump::PrintStackTrace(void* pOsSpecificData)
{
  _EXCEPTION_POINTERS* pExceptionInfo = (_EXCEPTION_POINTERS*)pOsSpecificData;

  xiiLog::Printf("***Unhandled Exception:***\n");
  xiiLog::Printf("Exception: %08x", (xiiUInt32)pExceptionInfo->ExceptionRecord->ExceptionCode);

  {
    xiiLog::Printf("\n\n***Stack Trace:***\n");
    void*              pBuffer[64];
    xiiArrayPtr<void*> tempTrace(pBuffer);
    const xiiUInt32    uiNumTraces = xiiStackTracer::GetStackTrace(tempTrace, pExceptionInfo->ContextRecord);

    xiiStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &PrintHelper);
  }
}
