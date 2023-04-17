#include <Core/CorePCH.h>

#include <Core/Console/LuaInterpreter.h>
#include <Core/Console/QuakeConsole.h>
#include <Core/Scripting/LuaWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

static void AllowScriptCVarAccess(xiiLuaWrapper& Script);

static const xiiString GetNextWord(xiiStringView& sString)
{
  const char* szStartWord = xiiStringUtils::SkipCharacters(sString.GetStartPointer(), xiiStringUtils::IsWhiteSpace, false);
  const char* szEndWord   = xiiStringUtils::FindWordEnd(szStartWord, xiiStringUtils::IsIdentifierDelimiter_C_Code, true);

  sString = xiiStringView(szEndWord);

  return xiiStringView(szStartWord, szEndWord);
}

static xiiString GetRestWords(xiiStringView sString)
{
  return xiiStringUtils::SkipCharacters(sString.GetStartPointer(), xiiStringUtils::IsWhiteSpace, false);
}

static int LUAFUNC_ConsoleFunc(lua_State* state)
{
  xiiLuaWrapper s(state);

  xiiConsoleFunctionBase* pFunc = (xiiConsoleFunctionBase*)s.GetFunctionLightUserData();

  if (pFunc->GetNumParameters() != s.GetNumberOfFunctionParameters())
  {
    xiiLog::Error("Function '{0}' expects {1} parameters, {2} were provided.", pFunc->GetName(), pFunc->GetNumParameters(), s.GetNumberOfFunctionParameters());
    return s.ReturnToScript();
  }

  xiiHybridArray<xiiVariant, 8> m_Params;
  m_Params.SetCount(pFunc->GetNumParameters());

  for (xiiUInt32 p = 0; p < pFunc->GetNumParameters(); ++p)
  {
    switch (pFunc->GetParameterType(p))
    {
      case xiiVariant::Type::Bool:
        m_Params[p] = s.GetBoolParameter(p);
        break;
      case xiiVariant::Type::Int8:
      case xiiVariant::Type::Int16:
      case xiiVariant::Type::Int32:
      case xiiVariant::Type::Int64:
      case xiiVariant::Type::UInt8:
      case xiiVariant::Type::UInt16:
      case xiiVariant::Type::UInt32:
      case xiiVariant::Type::UInt64:
        m_Params[p] = s.GetIntParameter(p);
        break;
      case xiiVariant::Type::Float:
        m_Params[p] = s.GetFloatParameter(p);
        break;
      case xiiVariant::Type::Double:
        m_Params[p] = s.GetDoubleParameter(p);
        break;
      case xiiVariant::Type::String:
        m_Params[p] = s.GetStringParameter(p);
        break;
      default:
        xiiLog::Error("Function '{0}': Type of parameter {1} is not supported by the Lua interpreter.", pFunc->GetName(), p);
        return s.ReturnToScript();
    }
  }

  if (!m_Params.IsEmpty())
    pFunc->Call(xiiArrayPtr<xiiVariant>(&m_Params[0], m_Params.GetCount())).IgnoreResult();
  else
    pFunc->Call(xiiArrayPtr<xiiVariant>()).IgnoreResult();

  return s.ReturnToScript();
}

static void SanitizeCVarNames(xiiStringBuilder& sCommand)
{
  xiiStringBuilder sanitizedCVarName;

  for (const xiiCVar* pCVar = xiiCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    sanitizedCVarName = pCVar->GetName();
    sanitizedCVarName.ReplaceAll(".", "_");

    sCommand.ReplaceAll(pCVar->GetName(), sanitizedCVarName);
  }
}

static void UnSanitizeCVarName(xiiStringBuilder& cvarName)
{
  xiiStringBuilder sanitizedCVarName;

  for (const xiiCVar* pCVar = xiiCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    sanitizedCVarName = pCVar->GetName();
    sanitizedCVarName.ReplaceAll(".", "_");

    if (cvarName == sanitizedCVarName)
    {
      cvarName = pCVar->GetName();
      return;
    }
  }
}

void xiiCommandInterpreterLua::Interpret(xiiCommandInterpreterState& inout_State)
{
  inout_State.m_sOutput.Clear();

  xiiStringBuilder sRealCommand = inout_State.m_sInput;

  if (sRealCommand.IsEmpty())
  {
    inout_State.AddOutputLine("");
    return;
  }

  sRealCommand.Trim(" \t\n\r");
  xiiStringBuilder sSanitizedCommand = sRealCommand;
  SanitizeCVarNames(sSanitizedCommand);

  xiiStringView sCommandIt = sSanitizedCommand;

  const xiiString  sSanitizedVarName = GetNextWord(sCommandIt);
  xiiStringBuilder sRealVarName      = sSanitizedVarName;
  UnSanitizeCVarName(sRealVarName);

  while (xiiStringUtils::IsWhiteSpace(sCommandIt.GetCharacter()))
  {
    sCommandIt.Shrink(1, 0);
  }

  const bool bSetValue = sCommandIt.StartsWith("=");

  if (bSetValue)
  {
    sCommandIt.Shrink(1, 0);
  }

  xiiStringBuilder sValue      = GetRestWords(sCommandIt);
  bool             bValueEmpty = sValue.IsEmpty();

  xiiStringBuilder sTemp;

  xiiLuaWrapper Script;
  AllowScriptCVarAccess(Script);

  // Register all ConsoleFunctions
  {
    xiiConsoleFunctionBase* pFunc = xiiConsoleFunctionBase::GetFirstInstance();
    while (pFunc)
    {
      Script.RegisterCFunction(pFunc->GetName().GetData(sTemp), LUAFUNC_ConsoleFunc, pFunc);

      pFunc = pFunc->GetNextInstance();
    }
  }

  sTemp = "> ";
  sTemp.Append(sRealCommand);
  inout_State.AddOutputLine(sTemp, xiiConsoleString::Type::Executed);

  xiiCVar* pCVAR = xiiCVar::FindCVarByName(sRealVarName.GetData());
  if (pCVAR != nullptr)
  {
    if ((bSetValue) && (sValue == "") && (pCVAR->GetType() == xiiCVarType::Bool))
    {
      // Someone typed "myvar =" -> on bools this is the short form for "myvar = not myvar" (toggle), so insert the rest here.

      bValueEmpty = false;

      sSanitizedCommand.AppendFormat(" not {0}", sSanitizedVarName);
    }

    if (bSetValue && !bValueEmpty)
    {
      xiiMuteLog muteLog;

      if (Script.ExecuteString(sSanitizedCommand, "console", &muteLog).Failed())
      {
        inout_State.AddOutputLine("  Error Executing Command.", xiiConsoleString::Type::Error);
        return;
      }
      else
      {
        if (pCVAR->GetFlags().IsAnySet(xiiCVarFlags::RequiresRestart))
        {
          inout_State.AddOutputLine("  This change takes only effect after a restart.", xiiConsoleString::Type::Note);
        }

        sTemp.Format("  {0} = {1}", sRealVarName, xiiQuakeConsole::GetFullInfoAsString(pCVAR));
        inout_State.AddOutputLine(sTemp, xiiConsoleString::Type::Success);
      }
    }
    else
    {
      sTemp.Format("{0} = {1}", sRealVarName, xiiQuakeConsole::GetFullInfoAsString(pCVAR));
      inout_State.AddOutputLine(sTemp);

      if (!pCVAR->GetDescription().IsEmpty())
      {
        sTemp.Format("  Description: {0}", pCVAR->GetDescription());
        inout_State.AddOutputLine(sTemp, xiiConsoleString::Type::Success);
      }
      else
        inout_State.AddOutputLine("  No Description available.", xiiConsoleString::Type::Success);
    }

    return;
  }
  else
  {
    xiiMuteLog muteLog;

    if (Script.ExecuteString(sSanitizedCommand, "console", &muteLog).Failed())
    {
      inout_State.AddOutputLine("  Error Executing Command.", xiiConsoleString::Type::Error);
      return;
    }
  }
}

static int LUAFUNC_ReadCVAR(lua_State* state)
{
  xiiLuaWrapper s(state);

  xiiStringBuilder cvarName = s.GetStringParameter(0);
  UnSanitizeCVarName(cvarName);

  xiiCVar* pCVar = xiiCVar::FindCVarByName(cvarName);

  if (pCVar == nullptr)
  {
    s.PushReturnValueNil();
    return s.ReturnToScript();
  }

  switch (pCVar->GetType())
  {
    case xiiCVarType::Int:
    {
      xiiCVarInt* pVar = (xiiCVarInt*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case xiiCVarType::Bool:
    {
      xiiCVarBool* pVar = (xiiCVarBool*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case xiiCVarType::Float:
    {
      xiiCVarFloat* pVar = (xiiCVarFloat*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case xiiCVarType::Double:
    {
      xiiCVarDouble* pVar = (xiiCVarDouble*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case xiiCVarType::String:
    {
      xiiCVarString* pVar = (xiiCVarString*)pCVar;
      s.PushReturnValue(pVar->GetValue().GetData());
    }
    break;
    case xiiCVarType::ENUM_COUNT:
      break;
  }

  return s.ReturnToScript();
}


static int LUAFUNC_WriteCVAR(lua_State* state)
{
  xiiLuaWrapper s(state);

  xiiStringBuilder cvarName = s.GetStringParameter(0);
  UnSanitizeCVarName(cvarName);

  xiiCVar* pCVar = xiiCVar::FindCVarByName(cvarName);

  if (pCVar == nullptr)
  {
    s.PushReturnValue(false);
    return s.ReturnToScript();
  }

  s.PushReturnValue(true);

  switch (pCVar->GetType())
  {
    case xiiCVarType::Int:
    {
      xiiCVarInt* pVar = (xiiCVarInt*)pCVar;
      *pVar            = s.GetIntParameter(1);
    }
    break;
    case xiiCVarType::Bool:
    {
      xiiCVarBool* pVar = (xiiCVarBool*)pCVar;
      *pVar             = s.GetBoolParameter(1);
    }
    break;
    case xiiCVarType::Float:
    {
      xiiCVarFloat* pVar = (xiiCVarFloat*)pCVar;
      *pVar              = s.GetFloatParameter(1);
    }
    break;
    case xiiCVarType::Double:
    {
      xiiCVarDouble* pVar = (xiiCVarDouble*)pCVar;
      *pVar               = s.GetDoubleParameter(1);
    }
    break;
    case xiiCVarType::String:
    {
      xiiCVarString* pVar = (xiiCVarString*)pCVar;
      *pVar               = s.GetStringParameter(1);
    }
    break;
    case xiiCVarType::ENUM_COUNT:
      break;
  }

  return s.ReturnToScript();
}

static void AllowScriptCVarAccess(xiiLuaWrapper& Script)
{
  Script.RegisterCFunction("ReadCVar", LUAFUNC_ReadCVAR);
  Script.RegisterCFunction("WriteCVar", LUAFUNC_WriteCVAR);

  xiiStringBuilder sInit = "\
function readcvar (t, key)\n\
return (ReadCVar (key))\n\
end\n\
\n\
function writecvar (t, key, value)\n\
if not WriteCVar (key, value) then\n\
rawset (t, key, value or false)\n\
end\n\
end\n\
\n\
setmetatable (_G, {\n\
__newindex = writecvar,\n\
__index = readcvar,\n\
__metatable = \"Access Denied\",\n\
})";

  Script.ExecuteString(sInit.GetData()).IgnoreResult();
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT


XII_STATICLINK_FILE(Core, Core_Console_Implementation_LuaInterpreter);
