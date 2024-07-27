#include <Core/CorePCH.h>

#include <Core/Scripting/DuktapeHelper.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duktape.h>
#  include <Foundation/IO/FileSystem/FileReader.h>

static_assert(xiiDuktapeTypeMask::None == DUK_TYPE_MASK_NONE);
static_assert(xiiDuktapeTypeMask::Undefined == DUK_TYPE_MASK_UNDEFINED);
static_assert(xiiDuktapeTypeMask::Null == DUK_TYPE_MASK_NULL);
static_assert(xiiDuktapeTypeMask::Bool == DUK_TYPE_MASK_BOOLEAN);
static_assert(xiiDuktapeTypeMask::Number == DUK_TYPE_MASK_NUMBER);
static_assert(xiiDuktapeTypeMask::String == DUK_TYPE_MASK_STRING);
static_assert(xiiDuktapeTypeMask::Object == DUK_TYPE_MASK_OBJECT);
static_assert(xiiDuktapeTypeMask::Buffer == DUK_TYPE_MASK_BUFFER);
static_assert(xiiDuktapeTypeMask::Pointer == DUK_TYPE_MASK_POINTER);

#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG)


void xiiDuktapeHelper::EnableStackChangeVerification() const
{
  m_bVerifyStackChange = true;
}

#  endif

xiiDuktapeHelper::xiiDuktapeHelper(duk_context* pContext) :
  m_pContext(pContext)
{
#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  if (m_pContext)
  {
    m_bVerifyStackChange = false;
    m_iStackTopAtStart   = duk_get_top(m_pContext);
  }
#  endif
}

xiiDuktapeHelper::xiiDuktapeHelper(const xiiDuktapeHelper& rhs) :
  xiiDuktapeHelper(rhs.GetContext())
{
}

xiiDuktapeHelper::~xiiDuktapeHelper() = default;

void xiiDuktapeHelper::operator=(const xiiDuktapeHelper& rhs)
{
  if (this == &rhs)
    return;

  *this = xiiDuktapeHelper(rhs.GetContext());
}

#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
void xiiDuktapeHelper::VerifyExpectedStackChange(xiiInt32 iExpectedStackChange, const char* szFile, xiiUInt32 uiLine, const char* szFunction) const
{
  if (m_bVerifyStackChange && m_pContext)
  {
    const xiiInt32 iCurTop      = duk_get_top(m_pContext);
    const xiiInt32 iStackChange = iCurTop - m_iStackTopAtStart;

    if (iStackChange != iExpectedStackChange)
    {
      xiiLog::Error("{}:{} ({}): Stack change {} != {}", szFile, uiLine, szFunction, iStackChange, iExpectedStackChange);
    }
  }
}
#  endif

void xiiDuktapeHelper::Error(const xiiFormatString& text)
{
  xiiStringBuilder tmp;
  duk_error(m_pContext, DUK_ERR_ERROR, text.GetTextCStr(tmp));
}

void xiiDuktapeHelper::LogStackTrace(xiiInt32 iErrorObjIdx)
{
  if (duk_is_error(m_pContext, iErrorObjIdx))
  {
    XII_LOG_BLOCK("Stack Trace");

    duk_get_prop_string(m_pContext, iErrorObjIdx, "stack");

    const xiiStringBuilder            stack = duk_safe_to_string(m_pContext, iErrorObjIdx);
    xiiHybridArray<xiiStringView, 32> lines;
    stack.Split(false, lines, "\n", "\r");

    for (xiiStringView line : lines)
    {
      xiiLog::Dev("{}", line);
    }

    duk_pop(m_pContext);
  }
}

void xiiDuktapeHelper::PopStack(xiiUInt32 n /*= 1*/)
{
  duk_pop_n(m_pContext, n);
}

void xiiDuktapeHelper::PushGlobalObject()
{
  duk_push_global_object(m_pContext); // [ global ]
}

void xiiDuktapeHelper::PushGlobalStash()
{
  duk_push_global_stash(m_pContext); // [ stash ]
}

xiiResult xiiDuktapeHelper::PushLocalObject(const char* szName, xiiInt32 iParentObjectIndex /* = -1*/)
{
  duk_require_top_index(m_pContext);

  if (duk_get_prop_string(m_pContext, iParentObjectIndex, szName) == false) // [ obj/undef ]
  {
    duk_pop(m_pContext); // [ ]
    return XII_FAILURE;
  }

  // [ object ]
  return XII_SUCCESS;
}

bool xiiDuktapeHelper::HasProperty(const char* szPropertyName, xiiInt32 iParentObjectIndex /*= -1*/) const
{
  return duk_is_object(m_pContext, iParentObjectIndex) && duk_has_prop_string(m_pContext, iParentObjectIndex, szPropertyName);
}

bool xiiDuktapeHelper::GetBoolProperty(const char* szPropertyName, bool bFallback, xiiInt32 iParentObjectIndex /*= -1*/) const
{
  bool result = bFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_boolean_default(m_pContext, -1, bFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

xiiInt32 xiiDuktapeHelper::GetIntProperty(const char* szPropertyName, xiiInt32 iFallback, xiiInt32 iParentObjectIndex /*= -1*/) const
{
  xiiInt32 result = iFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_int_default(m_pContext, -1, iFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

xiiUInt32 xiiDuktapeHelper::GetUIntProperty(const char* szPropertyName, xiiUInt32 uiFallback, xiiInt32 iParentObjectIndex /*= -1*/) const
{
  xiiUInt32 result = uiFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_uint_default(m_pContext, -1, uiFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

float xiiDuktapeHelper::GetFloatProperty(const char* szPropertyName, float fFallback, xiiInt32 iParentObjectIndex /*= -1*/) const
{
  return static_cast<float>(GetNumberProperty(szPropertyName, fFallback, iParentObjectIndex));
}

double xiiDuktapeHelper::GetNumberProperty(const char* szPropertyName, double fFallback, xiiInt32 iParentObjectIndex /*= -1*/) const
{
  double result = fFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_number_default(m_pContext, -1, fFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

const char* xiiDuktapeHelper::GetStringProperty(const char* szPropertyName, const char* szFallback, xiiInt32 iParentObjectIndex /*= -1*/) const
{
  const char* result = szFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_string_default(m_pContext, -1, szFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

void xiiDuktapeHelper::SetBoolProperty(const char* szPropertyName, bool value, xiiInt32 iParentObjectIndex /*= -1*/) const
{
  xiiDuktapeHelper duk(m_pContext);

  duk_push_boolean(m_pContext, value); // [ value ]

  if (iParentObjectIndex >= 0)
    duk_put_prop_string(m_pContext, iParentObjectIndex, szPropertyName); // [ ]
  else
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szPropertyName); // [ ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void xiiDuktapeHelper::SetNumberProperty(const char* szPropertyName, double value, xiiInt32 iParentObjectIndex /*= -1*/) const
{
  xiiDuktapeHelper duk(m_pContext);

  duk_push_number(m_pContext, value); // [ value ]

  if (iParentObjectIndex >= 0)
    duk_put_prop_string(m_pContext, iParentObjectIndex, szPropertyName); // [ ]
  else
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szPropertyName); // [ ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void xiiDuktapeHelper::SetStringProperty(const char* szPropertyName, const char* value, xiiInt32 iParentObjectIndex /*= -1*/) const
{
  xiiDuktapeHelper duk(m_pContext);

  duk_push_string(m_pContext, value); // [ value ]

  if (iParentObjectIndex >= 0)
    duk_put_prop_string(m_pContext, iParentObjectIndex, szPropertyName); // [ ]
  else
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szPropertyName); // [ ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void xiiDuktapeHelper::SetCustomProperty(const char* szPropertyName, xiiInt32 iParentObjectIndex /*= -1*/) const
{
  xiiDuktapeHelper duk(m_pContext); // [ value ]

  if (iParentObjectIndex >= 0)
    duk_put_prop_string(m_pContext, iParentObjectIndex, szPropertyName); // [ ]
  else
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szPropertyName); // [ ]

  XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, -1);
}

void xiiDuktapeHelper::StorePointerInStash(const char* szKey, void* pPointer)
{
  duk_push_global_stash(m_pContext);                                                      // [ stash ]
  *reinterpret_cast<void**>(duk_push_fixed_buffer(m_pContext, sizeof(void*))) = pPointer; // [ stash buffer ]
  duk_put_prop_string(m_pContext, -2, szKey);                                             // [ stash ]
  duk_pop(m_pContext);                                                                    // [ ]
}

void* xiiDuktapeHelper::RetrievePointerFromStash(const char* szKey) const
{
  void* pPointer = nullptr;

  duk_push_global_stash(m_pContext); // [ stash ]

  if (duk_get_prop_string(m_pContext, -1, szKey)) // [ stash obj/undef ]
  {
    XII_ASSERT_DEBUG(duk_is_buffer(m_pContext, -1), "Object '{}' in stash is not a buffer", szKey);

    pPointer = *reinterpret_cast<void**>(duk_get_buffer(m_pContext, -1, nullptr)); // [ stash obj/undef ]
  }

  duk_pop_2(m_pContext); // [ ]

  return pPointer;
}

void xiiDuktapeHelper::StoreStringInStash(const char* szKey, const char* value)
{
  duk_push_global_stash(m_pContext);          // [ stash ]
  duk_push_string(m_pContext, value);         // [ stash value ]
  duk_put_prop_string(m_pContext, -2, szKey); // [ stash ]
  duk_pop(m_pContext);                        // [ ]
}

const char* xiiDuktapeHelper::RetrieveStringFromStash(const char* szKey, const char* szFallback /*= nullptr*/) const
{
  duk_push_global_stash(m_pContext); // [ stash ]

  if (!duk_get_prop_string(m_pContext, -1, szKey)) // [ stash string/undef ]
  {
    duk_pop_2(m_pContext); // [ ]
    return szFallback;
  }

  szFallback = duk_get_string_default(m_pContext, -1, szFallback); // [ stash string ]
  duk_pop_2(m_pContext);                                           // [ ]

  return szFallback;
}

bool xiiDuktapeHelper::IsOfType(xiiBitflags<xiiDuktapeTypeMask> mask, xiiInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, mask.GetValue());
}

bool xiiDuktapeHelper::IsBool(xiiInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_BOOLEAN);
}

bool xiiDuktapeHelper::IsNumber(xiiInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_NUMBER);
}

bool xiiDuktapeHelper::IsString(xiiInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_STRING);
}

bool xiiDuktapeHelper::IsNull(xiiInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_NULL);
}

bool xiiDuktapeHelper::IsUndefined(xiiInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_UNDEFINED);
}

bool xiiDuktapeHelper::IsObject(xiiInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_OBJECT);
}

bool xiiDuktapeHelper::IsBuffer(xiiInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_BUFFER);
}

bool xiiDuktapeHelper::IsPointer(xiiInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_POINTER);
}

bool xiiDuktapeHelper::IsNullOrUndefined(xiiInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_NULL | DUK_TYPE_MASK_UNDEFINED);
}

void xiiDuktapeHelper::RegisterGlobalFunction(const char* szFunctionName, duk_c_function function, xiiUInt8 uiNumArguments, xiiInt16 iMagicValue /*= 0*/)
{
  // TODO: could store iFuncIdx for faster function calls

  duk_push_global_object(m_pContext);                                                // [ global ]
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, function, uiNumArguments); // [ global func ]
  duk_set_magic(m_pContext, -1, iMagicValue);                                        // [ global func ]
  duk_put_prop_string(m_pContext, -2, szFunctionName);                               // [ global ]
  duk_pop(m_pContext);                                                               // [ ]
}

void xiiDuktapeHelper::RegisterGlobalFunctionWithVarArgs(const char* szFunctionName, duk_c_function function, xiiInt16 iMagicValue /*= 0*/)
{
  // TODO: could store iFuncIdx for faster function calls

  duk_push_global_object(m_pContext);                                             // [ global ]
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, function, DUK_VARARGS); // [ global func ]
  duk_set_magic(m_pContext, -1, iMagicValue);                                     // [ global func ]
  duk_put_prop_string(m_pContext, -2, szFunctionName);                            // [ global ]
  duk_pop(m_pContext);                                                            // [ ]
}

void xiiDuktapeHelper::RegisterObjectFunction(const char* szFunctionName, duk_c_function function, xiiUInt8 uiNumArguments, xiiInt32 iParentObjectIndex /*= -1*/, xiiInt16 iMagicValue /*= 0*/)
{
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, function, uiNumArguments); // [ func ]
  duk_set_magic(m_pContext, -1, iMagicValue);                                        // [ func ]

  if (iParentObjectIndex < 0)
  {
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szFunctionName); // [ ]
  }
  else
  {
    duk_put_prop_string(m_pContext, iParentObjectIndex, szFunctionName); // [ ]
  }
}

xiiResult xiiDuktapeHelper::PrepareGlobalFunctionCall(const char* szFunctionName)
{
  if (!duk_get_global_string(m_pContext, szFunctionName)) // [ func/undef ]
    goto Failed;

  if (!duk_is_function(m_pContext, -1)) // [ func ]
    goto Failed;

  m_iPushedValues = 0;
  return XII_SUCCESS; // [ func ]

Failed:
  duk_pop(m_pContext); // [ ]
  return XII_FAILURE;
}

xiiResult xiiDuktapeHelper::PrepareObjectFunctionCall(const char* szFunctionName, xiiInt32 iParentObjectIndex /*= -1*/)
{
  duk_require_top_index(m_pContext);

  if (!duk_get_prop_string(m_pContext, iParentObjectIndex, szFunctionName)) // [ func/undef ]
    goto Failed;

  if (!duk_is_function(m_pContext, -1)) // [ func ]
    goto Failed;

  m_iPushedValues = 0;
  return XII_SUCCESS; // [ func ]

Failed:
  duk_pop(m_pContext); // [ ]
  return XII_FAILURE;
}

xiiResult xiiDuktapeHelper::CallPreparedFunction()
{
  if (duk_pcall(m_pContext, m_iPushedValues) == DUK_EXEC_SUCCESS) // [ func n-args ] -> [ result/error ]
  {
    return XII_SUCCESS; // [ result ]
  }
  else
  {
    xiiLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));

    LogStackTrace(-1);

    return XII_FAILURE; // [ error ]
  }
}

xiiResult xiiDuktapeHelper::PrepareMethodCall(const char* szMethodName, xiiInt32 iParentObjectIndex /*= -1*/)
{
  if (!duk_get_prop_string(m_pContext, iParentObjectIndex, szMethodName)) // [ func/undef ]
    goto Failed;

  if (!duk_is_function(m_pContext, -1)) // [ func ]
    goto Failed;

  if (iParentObjectIndex < 0)
  {
    duk_dup(m_pContext, iParentObjectIndex - 1); // [ func this ]
  }
  else
  {
    duk_dup(m_pContext, iParentObjectIndex); // [ func this ]
  }

  m_iPushedValues = 0;
  return XII_SUCCESS; // [ func this ]

Failed:
  duk_pop(m_pContext); // [ ]
  return XII_FAILURE;
}

xiiResult xiiDuktapeHelper::CallPreparedMethod()
{
  if (duk_pcall_method(m_pContext, m_iPushedValues) == DUK_EXEC_SUCCESS) // [ func this n-args ] -> [ result/error ]
  {
    return XII_SUCCESS; // [ result ]
  }
  else
  {
    xiiLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));

    LogStackTrace(-1);

    return XII_FAILURE; // [ error ]
  }
}

void xiiDuktapeHelper::PushInt(xiiInt32 iParam)
{
  duk_push_int(m_pContext, iParam); // [ value ]
  ++m_iPushedValues;
}

void xiiDuktapeHelper::PushUInt(xiiUInt32 uiParam)
{
  duk_push_uint(m_pContext, uiParam); // [ value ]
  ++m_iPushedValues;
}

void xiiDuktapeHelper::PushBool(bool bParam)
{
  duk_push_boolean(m_pContext, bParam); // [ value ]
  ++m_iPushedValues;
}

void xiiDuktapeHelper::PushNumber(double fParam)
{
  duk_push_number(m_pContext, fParam); // [ value ]
  ++m_iPushedValues;
}

void xiiDuktapeHelper::PushString(const xiiStringView& sParam)
{
  duk_push_lstring(m_pContext, sParam.GetStartPointer(), sParam.GetElementCount()); // [ value ]
  ++m_iPushedValues;
}

void xiiDuktapeHelper::PushNull()
{
  duk_push_null(m_pContext); // [ null ]
  ++m_iPushedValues;
}

void xiiDuktapeHelper::PushUndefined()
{
  duk_push_undefined(m_pContext); // [ undefined ]
  ++m_iPushedValues;
}

void xiiDuktapeHelper::PushCustom(xiiUInt32 uiNum)
{
  m_iPushedValues += uiNum;
}

bool xiiDuktapeHelper::GetBoolValue(xiiInt32 iStackElement, bool bFallback /*= false*/) const
{
  return duk_get_boolean_default(m_pContext, iStackElement, bFallback);
}

xiiInt32 xiiDuktapeHelper::GetIntValue(xiiInt32 iStackElement, xiiInt32 iFallback /*= 0*/) const
{
  return duk_get_int_default(m_pContext, iStackElement, iFallback);
}

xiiUInt32 xiiDuktapeHelper::GetUIntValue(xiiInt32 iStackElement, xiiUInt32 uiFallback /*= 0*/) const
{
  return duk_get_uint_default(m_pContext, iStackElement, uiFallback);
}

float xiiDuktapeHelper::GetFloatValue(xiiInt32 iStackElement, float fFallback /*= 0*/) const
{
  return static_cast<float>(duk_get_number_default(m_pContext, iStackElement, fFallback));
}

double xiiDuktapeHelper::GetNumberValue(xiiInt32 iStackElement, double fFallback /*= 0*/) const
{
  return duk_get_number_default(m_pContext, iStackElement, fFallback);
}

const char* xiiDuktapeHelper::GetStringValue(xiiInt32 iStackElement, const char* szFallback /*= ""*/) const
{
  return duk_get_string_default(m_pContext, iStackElement, szFallback);
}

xiiResult xiiDuktapeHelper::ExecuteString(const char* szString, const char* szDebugName /*= "eval"*/)
{
  duk_push_string(m_pContext, szDebugName);                       // [ filename ]
  if (duk_pcompile_string_filename(m_pContext, 0, szString) != 0) // [ function/error ]
  {
    XII_LOG_BLOCK("DukTape::ExecuteString", "Compilation failed");

    xiiLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1)); // [ error ]

    LogStackTrace(-1);

    // TODO: print out line by line
    xiiLog::Info("[duktape]Source: {0}", szString);

    duk_pop(m_pContext); // [ ]
    return XII_FAILURE;
  }

  // [ function ]

  if (duk_pcall(m_pContext, 0) != DUK_EXEC_SUCCESS) // [ result/error ]
  {
    XII_LOG_BLOCK("DukTape::ExecuteString", "Execution failed");

    xiiLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1)); // [ error ]

    LogStackTrace(-1);

    // TODO: print out line by line
    xiiLog::Info("[duktape]Source: {0}", szString);

    duk_pop(m_pContext); // [ ]
    return XII_FAILURE;
  }

  duk_pop(m_pContext); // [ ]
  return XII_SUCCESS;
}

xiiResult xiiDuktapeHelper::ExecuteStream(xiiStreamReader& ref_stream, const char* szDebugName)
{
  xiiStringBuilder source;
  source.ReadAll(ref_stream);

  return ExecuteString(source, szDebugName);
}

xiiResult xiiDuktapeHelper::ExecuteFile(const char* szFile)
{
  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(szFile));

  return ExecuteStream(file, szFile);
}

#endif

XII_STATICLINK_FILE(Core, Core_Scripting_Duktape_DuktapeHelper);
