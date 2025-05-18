#include <Core/CorePCH.h>

#include <Core/Scripting/LuaWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

xiiLuaWrapper::xiiLuaWrapper()
{
  m_bReleaseOnExit = true;
  m_pState         = nullptr;

  Clear();
}

xiiLuaWrapper::xiiLuaWrapper(lua_State* s)
{
  m_pState         = s;
  m_bReleaseOnExit = false;
}

xiiLuaWrapper::~xiiLuaWrapper()
{
  if (m_bReleaseOnExit)
    lua_close(m_pState);
}

void xiiLuaWrapper::Clear()
{
  XII_ASSERT_DEV(m_bReleaseOnExit, "Cannot clear a script that did not create the Lua state itself.");

  if (m_pState)
    lua_close(m_pState);

  m_pState = lua_newstate(lua_allocator, nullptr);

  luaL_openlibs(m_pState);
}

xiiResult xiiLuaWrapper::ExecuteString(const char* szString, const char* szDebugChunkName, xiiLogInterface* pLogInterface) const
{
  XII_ASSERT_DEV(m_States.m_iLuaReturnValues == 0, "xiiLuaWrapper::ExecuteString: You didn't discard the return-values of the previous script call. {0} Return-values were expected.", m_States.m_iLuaReturnValues);

  if (!pLogInterface)
    pLogInterface = xiiLog::GetThreadLocalLogSystem();

  int error = luaL_loadbuffer(m_pState, szString, xiiStringUtils::GetStringElementCount(szString), szDebugChunkName);

  if (error != LUA_OK)
  {
    XII_LOG_BLOCK("xiiLuaWrapper::ExecuteString");

    xiiLog::Error(pLogInterface, "[lua]Lua compile error: {0}", lua_tostring(m_pState, -1));
    xiiLog::Info(pLogInterface, "[luascript]Script: {0}", szString);

    return XII_FAILURE;
  }

  error = lua_pcall(m_pState, 0, 0, 0);

  if (error != LUA_OK)
  {
    XII_LOG_BLOCK("xiiLuaWrapper::ExecuteString");

    xiiLog::Error(pLogInterface, "[lua]Lua error: {0}", lua_tostring(m_pState, -1));
    xiiLog::Info(pLogInterface, "[luascript]Script: {0}", szString);

    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void* xiiLuaWrapper::lua_allocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
  XII_IGNORE_UNUSED(ud);

  /// \todo Create optimized allocator.

  if (nsize == 0)
  {
    delete[] (xiiUInt8*)ptr;
    return (nullptr);
  }

  xiiUInt8* ucPtr = new xiiUInt8[nsize];

  if (ptr != nullptr)
  {
    xiiMemoryUtils::Copy(ucPtr, (xiiUInt8*)ptr, xiiUInt32(osize < nsize ? osize : nsize));

    delete[] (xiiUInt8*)ptr;
  }

  return ((void*)ucPtr);
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT

XII_STATICLINK_FILE(Core, Core_Scripting_LuaWrapper_Initialize);
