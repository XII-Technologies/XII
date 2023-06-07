#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

struct duk_hthread;

using duk_context    = duk_hthread;
using duk_c_function = xiiInt32 (*)(duk_context* ctx);

struct xiiDuktapeTypeMask
{
  using StorageType = xiiUInt32;

  enum Enum
  {
    None      = XII_BIT(0), ///< no value, e.g. invalid index
    Undefined = XII_BIT(1), ///< ECMAScript undefined
    Null      = XII_BIT(2), ///< ECMAScript null
    Bool      = XII_BIT(3), ///< boolean, true or false
    Number    = XII_BIT(4), ///< any number, stored as a double
    String    = XII_BIT(5), ///< ECMAScript string: CESU-8 / extended UTF-8 encoded
    Object    = XII_BIT(6), ///< ECMAScript object: includes objects, arrays, functions, threads
    Buffer    = XII_BIT(7), ///< fixed or dynamic, garbage collected byte buffer
    Pointer   = XII_BIT(8)  ///< raw void pointer
  };

  struct Bits
  {
    StorageType None : 1;
    StorageType Undefined : 1;
    StorageType Null : 1;
    StorageType Bool : 1;
    StorageType Number : 1;
    StorageType String : 1;
    StorageType Object : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiDuktapeTypeMask);

#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG)

#    define XII_DUK_VERIFY_STACK(duk, ExpectedStackChange) \
      duk.EnableStackChangeVerification();                 \
      duk.VerifyExpectedStackChange(ExpectedStackChange, XII_SOURCE_FILE, XII_SOURCE_LINE, XII_SOURCE_FUNCTION);

#    define XII_DUK_RETURN_AND_VERIFY_STACK(duk, ReturnCode, ExpectedStackChange) \
      {                                                                           \
        auto ret = ReturnCode;                                                    \
        XII_DUK_VERIFY_STACK(duk, ExpectedStackChange);                           \
        return ret;                                                               \
      }

#    define XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, ExpectedStackChange) \
      XII_DUK_VERIFY_STACK(duk, ExpectedStackChange);                      \
      return;


#  else

#    define XII_DUK_VERIFY_STACK(duk, ExpectedStackChange)

#    define XII_DUK_RETURN_AND_VERIFY_STACK(duk, ReturnCode, ExpectedStackChange) return ReturnCode;

#    define XII_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, ExpectedStackChange) return;

#  endif

class XII_CORE_DLL xiiDuktapeHelper
{
public:
  xiiDuktapeHelper(duk_context* pContext);
  xiiDuktapeHelper(const xiiDuktapeHelper& rhs);
  ~xiiDuktapeHelper();
  void operator=(const xiiDuktapeHelper& rhs);

  /// \name Basics
  ///@{

  /// \brief Returns the raw Duktape context for custom operations.
  XII_ALWAYS_INLINE duk_context* GetContext() const { return m_pContext; }

  /// \brief Implicit conversion to duk_context*
  XII_ALWAYS_INLINE operator duk_context*() const { return m_pContext; }

#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  void VerifyExpectedStackChange(xiiInt32 iExpectedStackChange, const char* szFile, xiiUInt32 uiLine, const char* szFunction) const;
#  endif

  ///@}
  /// \name Error Handling
  ///@{

  void Error(const xiiFormatString& text);

  void LogStackTrace(xiiInt32 iErrorObjIdx);


  ///@}
  /// \name Objects / Stash
  ///@{

  void PopStack(xiiUInt32 n = 1);

  void PushGlobalObject();

  void PushGlobalStash();

  xiiResult PushLocalObject(const char* szName, xiiInt32 iParentObjectIndex = -1);

  ///@}
  /// \name Object Properties
  ///@{

  bool HasProperty(const char* szPropertyName, xiiInt32 iParentObjectIndex = -1) const;

  bool        GetBoolProperty(const char* szPropertyName, bool bFallback, xiiInt32 iParentObjectIndex = -1) const;
  xiiInt32    GetIntProperty(const char* szPropertyName, xiiInt32 iFallback, xiiInt32 iParentObjectIndex = -1) const;
  xiiUInt32   GetUIntProperty(const char* szPropertyName, xiiUInt32 uiFallback, xiiInt32 iParentObjectIndex = -1) const;
  float       GetFloatProperty(const char* szPropertyName, float fFallback, xiiInt32 iParentObjectIndex = -1) const;
  double      GetNumberProperty(const char* szPropertyName, double fFallback, xiiInt32 iParentObjectIndex = -1) const;
  const char* GetStringProperty(const char* szPropertyName, const char* szFallback, xiiInt32 iParentObjectIndex = -1) const;

  void SetBoolProperty(const char* szPropertyName, bool value, xiiInt32 iParentObjectIndex = -1) const;
  void SetNumberProperty(const char* szPropertyName, double value, xiiInt32 iParentObjectIndex = -1) const;
  void SetStringProperty(const char* szPropertyName, const char* value, xiiInt32 iParentObjectIndex = -1) const;

  /// \note If a negative parent index is given, the parent object taken is actually ParentIdx - 1 (obj at idx -1 is the custom object to use)
  void SetCustomProperty(const char* szPropertyName, xiiInt32 iParentObjectIndex = -1) const;


  ///@}
  /// \name Global State
  ///@{

  void  StorePointerInStash(const char* szKey, void* pPointer);
  void* RetrievePointerFromStash(const char* szKey) const;

  void        StoreStringInStash(const char* szKey, const char* value);
  const char* RetrieveStringFromStash(const char* szKey, const char* szFallback = nullptr) const;

  ///@}
  /// \name Type Checks
  ///@{

  bool IsOfType(xiiBitflags<xiiDuktapeTypeMask> mask, xiiInt32 iStackElement = -1) const;
  bool IsBool(xiiInt32 iStackElement = -1) const;
  bool IsNumber(xiiInt32 iStackElement = -1) const;
  bool IsString(xiiInt32 iStackElement = -1) const;
  bool IsNull(xiiInt32 iStackElement = -1) const;
  bool IsUndefined(xiiInt32 iStackElement = -1) const;
  bool IsObject(xiiInt32 iStackElement = -1) const;
  bool IsBuffer(xiiInt32 iStackElement = -1) const;
  bool IsPointer(xiiInt32 iStackElement = -1) const;
  bool IsNullOrUndefined(xiiInt32 iStackElement = -1) const;

  ///@}
  /// \name C Functions
  ///@{

  void RegisterGlobalFunction(const char* szFunctionName, duk_c_function function, xiiUInt8 uiNumArguments, xiiInt16 iMagicValue = 0);
  void RegisterGlobalFunctionWithVarArgs(const char* szFunctionName, duk_c_function function, xiiInt16 iMagicValue = 0);

  void RegisterObjectFunction(
    const char*    szFunctionName,
    duk_c_function function,
    xiiUInt8       uiNumArguments,
    xiiInt32       iParentObjectIndex = -1,
    xiiInt16       iMagicValue        = 0);

  xiiResult PrepareGlobalFunctionCall(const char* szFunctionName);
  xiiResult PrepareObjectFunctionCall(const char* szFunctionName, xiiInt32 iParentObjectIndex = -1);
  xiiResult CallPreparedFunction();

  xiiResult PrepareMethodCall(const char* szMethodName, xiiInt32 iParentObjectIndex = -1);
  xiiResult CallPreparedMethod();


  ///@}
  /// \name Values / Parameters
  ///@{

  void PushInt(xiiInt32 iParam);
  void PushUInt(xiiUInt32 uiParam);
  void PushBool(bool bParam);
  void PushNumber(double fParam);
  void PushString(const xiiStringView& sParam);
  void PushNull();
  void PushUndefined();
  void PushCustom(xiiUInt32 uiNum = 1);

  bool        GetBoolValue(xiiInt32 iStackElement, bool bFallback = false) const;
  xiiInt32    GetIntValue(xiiInt32 iStackElement, xiiInt32 iFallback = 0) const;
  xiiUInt32   GetUIntValue(xiiInt32 iStackElement, xiiUInt32 uiFallback = 0) const;
  float       GetFloatValue(xiiInt32 iStackElement, float fFallback = 0) const;
  double      GetNumberValue(xiiInt32 iStackElement, double fFallback = 0) const;
  const char* GetStringValue(xiiInt32 iStackElement, const char* szFallback = "") const;

  ///@}
  /// \name Executing Scripts
  ///@{

  xiiResult ExecuteString(const char* szString, const char* szDebugName = "eval");

  xiiResult ExecuteStream(xiiStreamReader& ref_stream, const char* szDebugName);

  xiiResult ExecuteFile(const char* szFile);

  ///@}

public:
#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  void EnableStackChangeVerification() const;
#  endif


protected:
  duk_context* m_pContext      = nullptr;
  xiiInt32     m_iPushedValues = 0;

#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  xiiInt32     m_iStackTopAtStart   = -1000;
  mutable bool m_bVerifyStackChange = false;

#  endif
};

#endif
