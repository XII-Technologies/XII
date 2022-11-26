#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Log(duk_context* pDuk);

xiiResult xiiTypeScriptBinding::Init_Log()
{
  m_Duk.RegisterGlobalFunction("__CPP_Log_Error", __CPP_Log, 1, xiiLogMsgType::ErrorMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_SeriousWarning", __CPP_Log, 1, xiiLogMsgType::SeriousWarningMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Warning", __CPP_Log, 1, xiiLogMsgType::WarningMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Success", __CPP_Log, 1, xiiLogMsgType::SuccessMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Info", __CPP_Log, 1, xiiLogMsgType::InfoMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Dev", __CPP_Log, 1, xiiLogMsgType::DevMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Debug", __CPP_Log, 1, xiiLogMsgType::DebugMsg);

  return XII_SUCCESS;
}

static int __CPP_Log(duk_context* pDuk)
{
  xiiDuktapeFunction duk(pDuk);
  const xiiInt16     iMagic = duk.GetFunctionMagicValue();

  switch (iMagic)
  {
    case xiiLogMsgType::ErrorMsg:
      xiiLog::Error(duk.GetStringValue(0));
      break;
    case xiiLogMsgType::SeriousWarningMsg:
      xiiLog::SeriousWarning(duk.GetStringValue(0));
      break;
    case xiiLogMsgType::WarningMsg:
      xiiLog::Warning(duk.GetStringValue(0));
      break;
    case xiiLogMsgType::SuccessMsg:
      xiiLog::Success(duk.GetStringValue(0));
      break;
    case xiiLogMsgType::InfoMsg:
      xiiLog::Info(duk.GetStringValue(0));
      break;
    case xiiLogMsgType::DevMsg:
      xiiLog::Dev(duk.GetStringValue(0));
      break;
    case xiiLogMsgType::DebugMsg:
      xiiLog::Debug(duk.GetStringValue(0));
      break;
  }

  return duk.ReturnVoid();
}
