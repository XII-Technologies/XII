#include <Core/CorePCH.h>

#include <Core/Scripting/DuktapeFunction.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duk_module_duktape.h>
#  include <Duktape/duktape.h>

xiiDuktapeFunction::xiiDuktapeFunction(duk_context* pExistingContext) :
  xiiDuktapeHelper(pExistingContext)
{
}

xiiDuktapeFunction::~xiiDuktapeFunction()
{
#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  if (m_bVerifyStackChange && !m_bDidReturnValue)
  {
    xiiLog::Error("You need to call one xiiDuktapeFunction::ReturnXY() and return its result from your C function.");
  }
#  endif
}

xiiUInt32 xiiDuktapeFunction::GetNumVarArgFunctionParameters() const
{
  return duk_get_top(GetContext());
}

xiiInt16 xiiDuktapeFunction::GetFunctionMagicValue() const
{
  return static_cast<xiiInt16>(duk_get_current_magic(GetContext()));
}

xiiInt32 xiiDuktapeFunction::ReturnVoid()
{
  XII_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  return 0;
}

xiiInt32 xiiDuktapeFunction::ReturnNull()
{
  XII_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_null(GetContext());
  return 1;
}

xiiInt32 xiiDuktapeFunction::ReturnUndefined()
{
  XII_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_undefined(GetContext());
  return 1;
}

xiiInt32 xiiDuktapeFunction::ReturnBool(bool value)
{
  XII_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_boolean(GetContext(), value);
  return 1;
}

xiiInt32 xiiDuktapeFunction::ReturnInt(xiiInt32 value)
{
  XII_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_int(GetContext(), value);
  return 1;
}

xiiInt32 xiiDuktapeFunction::ReturnUInt(xiiUInt32 value)
{
  XII_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_uint(GetContext(), value);
  return 1;
}

xiiInt32 xiiDuktapeFunction::ReturnFloat(float value)
{
  XII_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_number(GetContext(), value);
  return 1;
}

xiiInt32 xiiDuktapeFunction::ReturnNumber(double value)
{
  XII_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_number(GetContext(), value);
  return 1;
}

xiiInt32 xiiDuktapeFunction::ReturnString(const char* value)
{
  XII_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_string(GetContext(), value);
  return 1;
}

xiiInt32 xiiDuktapeFunction::ReturnCustom()
{
  XII_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  // push nothing, the user calls this because he pushed something custom already
  return 1;
}

#endif


XII_STATICLINK_FILE(Core, Core_Scripting_Duktape_DuktapeFunction);
