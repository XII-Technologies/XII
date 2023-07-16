#include <Core/CorePCH.h>

#include <Core/Scripting/LuaWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

void xiiLuaWrapper::RegisterCFunction(const char* szFunctionName, lua_CFunction function, void* pLightUserData) const
{
  lua_pushlightuserdata(m_pState, pLightUserData);
  lua_pushcclosure(m_pState, function, 1);
  lua_setglobal(m_pState, szFunctionName);
}

void* xiiLuaWrapper::GetFunctionLightUserData() const
{
  return lua_touserdata(m_pState, lua_upvalueindex(1));
}

bool xiiLuaWrapper::PrepareFunctionCall(const char* szFunctionName)
{
  XII_ASSERT_DEV(m_States.m_iLuaReturnValues == 0,
                 "xiiLuaWrapper::PrepareFunctionCall: You didn't discard the return-values of the previous script call. {0} Return-values "
                 "were expected.",
                 m_States.m_iLuaReturnValues);

  m_States.m_iParametersPushed = 0;

  if (m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szFunctionName);
  else
  {
    lua_pushstring(m_pState, szFunctionName);
    lua_gettable(m_pState, -2);
  }

  if (lua_isfunction(m_pState, -1) == 0)
  {
    lua_pop(m_pState, 1);
    return false;
  }

  return true;
}

xiiResult xiiLuaWrapper::CallPreparedFunction(xiiUInt32 uiExpectedReturnValues, xiiLogInterface* pLogInterface)
{
  m_States.m_iLuaReturnValues = uiExpectedReturnValues;

  // save the current states on a cheap stack
  const xiiScriptStates StackedStates = m_States;
  m_States                            = xiiScriptStates();

  if (pLogInterface == nullptr)
    pLogInterface = xiiLog::GetThreadLocalLogSystem();

  if (lua_pcall(m_pState, StackedStates.m_iParametersPushed, uiExpectedReturnValues, 0) != 0)
  {
    // restore the states to their previous values
    m_States = StackedStates;

    m_States.m_iLuaReturnValues = 0;

    xiiLog::Error(pLogInterface, "Script-function Call: {0}", lua_tostring(m_pState, -1));

    lua_pop(m_pState, 1); /* pop error message from the stack */
    return XII_FAILURE;
  }

  // before resetting the state, make sure the returned state has no stuff left
  XII_ASSERT_DEV((m_States.m_iLuaReturnValues == 0) && (m_States.m_iOpenTables == 0),
                 "After xiiLuaWrapper::CallPreparedFunction: Return values: {0}, Open Tables: {1}", m_States.m_iLuaReturnValues, m_States.m_iOpenTables);

  m_States = StackedStates;
  return XII_SUCCESS;
}

void xiiLuaWrapper::DiscardReturnValues()
{
  if (m_States.m_iLuaReturnValues == 0)
    return;

  lua_pop(m_pState, m_States.m_iLuaReturnValues);
  m_States.m_iLuaReturnValues = 0;
}

bool xiiLuaWrapper::IsReturnValueInt(xiiUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TNUMBER);
}

bool xiiLuaWrapper::IsReturnValueBool(xiiUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TBOOLEAN);
}

bool xiiLuaWrapper::IsReturnValueFloat(xiiUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TNUMBER);
}

bool xiiLuaWrapper::IsReturnValueDouble(xiiUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TNUMBER);
}

bool xiiLuaWrapper::IsReturnValueString(xiiUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TSTRING);
}

bool xiiLuaWrapper::IsReturnValueNil(xiiUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TNIL);
}

xiiInt32 xiiLuaWrapper::GetIntReturnValue(xiiUInt32 uiReturnValue) const
{
  return ((int)(lua_tointeger(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1)));
}

bool xiiLuaWrapper::GetBoolReturnValue(xiiUInt32 uiReturnValue) const
{
  return (lua_toboolean(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) != 0);
}

float xiiLuaWrapper::GetFloatReturnValue(xiiUInt32 uiReturnValue) const
{
  return ((float)(lua_tonumber(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1)));
}

double xiiLuaWrapper::GetDoubleReturnValue(xiiUInt32 uiReturnValue) const
{
  return (lua_tonumber(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1));
}

const char* xiiLuaWrapper::GetStringReturnValue(xiiUInt32 uiReturnValue) const
{
  return (lua_tostring(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1));
}


#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT



XII_STATICLINK_FILE(Core, Core_Scripting_LuaWrapper_CFunctions);
