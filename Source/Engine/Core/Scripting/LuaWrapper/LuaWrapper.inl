#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

#  pragma once

inline lua_State* xiiLuaWrapper::GetLuaState()
{
  return m_pState;
}

inline xiiInt32 xiiLuaWrapper::ReturnToScript() const
{
  return (m_States.m_iParametersPushed);
}

inline xiiUInt32 xiiLuaWrapper::GetNumberOfFunctionParameters() const
{
  return ((int)lua_gettop(m_pState));
}

inline bool xiiLuaWrapper::IsParameterBool(xiiUInt32 iParameter) const
{
  return (lua_type(m_pState, iParameter + s_iParamOffset) == LUA_TBOOLEAN);
}

inline bool xiiLuaWrapper::IsParameterFloat(xiiUInt32 iParameter) const
{
  return (lua_type(m_pState, iParameter + s_iParamOffset) == LUA_TNUMBER);
}

inline bool xiiLuaWrapper::IsParameterDouble(xiiUInt32 iParameter) const
{
  return (lua_type(m_pState, iParameter + s_iParamOffset) == LUA_TNUMBER);
}

inline bool xiiLuaWrapper::IsParameterInt(xiiUInt32 iParameter) const
{
  return (lua_type(m_pState, iParameter + s_iParamOffset) == LUA_TNUMBER);
}

inline bool xiiLuaWrapper::IsParameterString(xiiUInt32 iParameter) const
{
  return (lua_type(m_pState, iParameter + s_iParamOffset) == LUA_TSTRING);
}

inline bool xiiLuaWrapper::IsParameterNil(xiiUInt32 iParameter) const
{
  return (lua_type(m_pState, iParameter + s_iParamOffset) == LUA_TNIL);
}

inline bool xiiLuaWrapper::IsParameterTable(xiiUInt32 iParameter) const
{
  return (lua_type(m_pState, iParameter + s_iParamOffset) == LUA_TTABLE);
}

inline void xiiLuaWrapper::PushParameter(xiiInt32 iParameter)
{
  lua_pushinteger(m_pState, iParameter);
  m_States.m_iParametersPushed++;
}

inline void xiiLuaWrapper::PushParameter(bool bParameter)
{
  lua_pushboolean(m_pState, bParameter);
  m_States.m_iParametersPushed++;
}

inline void xiiLuaWrapper::PushParameter(float fParameter)
{
  lua_pushnumber(m_pState, fParameter);
  m_States.m_iParametersPushed++;
}

inline void xiiLuaWrapper::PushParameter(double fParameter)
{
  lua_pushnumber(m_pState, fParameter);
  m_States.m_iParametersPushed++;
}

inline void xiiLuaWrapper::PushParameter(const char* szParameter)
{
  lua_pushstring(m_pState, szParameter);
  m_States.m_iParametersPushed++;
}

inline void xiiLuaWrapper::PushParameter(const char* szParameter, xiiUInt32 length)
{
  lua_pushlstring(m_pState, szParameter, length);
  m_States.m_iParametersPushed++;
}

inline void xiiLuaWrapper::PushParameterNil()
{
  lua_pushnil(m_pState);
  m_States.m_iParametersPushed++;
}

inline void xiiLuaWrapper::PushReturnValue(xiiInt32 iParameter)
{
  lua_pushinteger(m_pState, iParameter);
  m_States.m_iParametersPushed++;
}

inline void xiiLuaWrapper::PushReturnValue(bool bParameter)
{
  lua_pushboolean(m_pState, bParameter);
  m_States.m_iParametersPushed++;
}

inline void xiiLuaWrapper::PushReturnValue(float fParameter)
{
  lua_pushnumber(m_pState, fParameter);
  m_States.m_iParametersPushed++;
}

inline void xiiLuaWrapper::PushReturnValue(double fParameter)
{
  lua_pushnumber(m_pState, fParameter);
  m_States.m_iParametersPushed++;
}

inline void xiiLuaWrapper::PushReturnValue(const char* szParameter)
{
  lua_pushstring(m_pState, szParameter);
  m_States.m_iParametersPushed++;
}

inline void xiiLuaWrapper::PushReturnValue(const char* szParameter, xiiUInt32 length)
{
  lua_pushlstring(m_pState, szParameter, length);
  m_States.m_iParametersPushed++;
}

inline void xiiLuaWrapper::PushReturnValueNil()
{
  lua_pushnil(m_pState);
  m_States.m_iParametersPushed++;
}

inline void xiiLuaWrapper::SetVariableNil(const char* szName) const
{
  lua_pushnil(m_pState);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void xiiLuaWrapper::SetVariable(const char* szName, xiiInt32 iValue) const
{
  lua_pushinteger(m_pState, iValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void xiiLuaWrapper::SetVariable(const char* szName, float fValue) const
{
  lua_pushnumber(m_pState, fValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void xiiLuaWrapper::SetVariable(const char* szName, double fValue) const
{
  lua_pushnumber(m_pState, fValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void xiiLuaWrapper::SetVariable(const char* szName, bool bValue) const
{
  lua_pushboolean(m_pState, bValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void xiiLuaWrapper::SetVariable(const char* szName, const char* szValue) const
{
  lua_pushstring(m_pState, szValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void xiiLuaWrapper::SetVariable(const char* szName, const char* szValue, xiiUInt32 len) const
{
  lua_pushlstring(m_pState, szValue, len);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void xiiLuaWrapper::PushTable(const char* szTableName, bool bGlobalTable)
{
  if (bGlobalTable || m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szTableName);
  else
  {
    lua_pushstring(m_pState, szTableName);
    lua_gettable(m_pState, -2);
  }

  m_States.m_iParametersPushed++;
}

inline int xiiLuaWrapper::GetIntParameter(xiiUInt32 iParameter) const
{
  return ((int)(lua_tointeger(m_pState, iParameter + s_iParamOffset)));
}

inline bool xiiLuaWrapper::GetBoolParameter(xiiUInt32 iParameter) const
{
  return (lua_toboolean(m_pState, iParameter + s_iParamOffset) != 0);
}

inline float xiiLuaWrapper::GetFloatParameter(xiiUInt32 iParameter) const
{
  return ((float)(lua_tonumber(m_pState, iParameter + s_iParamOffset)));
}

inline double xiiLuaWrapper::GetDoubleParameter(xiiUInt32 iParameter) const
{
  return (lua_tonumber(m_pState, iParameter + s_iParamOffset));
}

inline const char* xiiLuaWrapper::GetStringParameter(xiiUInt32 iParameter) const
{
  return (lua_tostring(m_pState, iParameter + s_iParamOffset));
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT
