#pragma once

#include <Core/CoreDLL.h>
#include <Core/Scripting/DuktapeHelper.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

class XII_CORE_DLL xiiDuktapeFunction final : public xiiDuktapeHelper
{
public:
  xiiDuktapeFunction(duk_context* pExistingContext);
  ~xiiDuktapeFunction();

  /// \name Retrieving function parameters
  ///@{

  /// Returns how many Parameters were passed to the called C-Function.
  xiiUInt32 GetNumVarArgFunctionParameters() const;

  xiiInt16 GetFunctionMagicValue() const;

  ///@}

  /// \name Returning values from C function
  ///@{

  xiiInt32 ReturnVoid();
  xiiInt32 ReturnNull();
  xiiInt32 ReturnUndefined();
  xiiInt32 ReturnBool(bool value);
  xiiInt32 ReturnInt(xiiInt32 value);
  xiiInt32 ReturnUInt(xiiUInt32 value);
  xiiInt32 ReturnFloat(float value);
  xiiInt32 ReturnNumber(double value);
  xiiInt32 ReturnString(const char* value);
  xiiInt32 ReturnCustom();

  ///@}

private:
  bool m_bDidReturnValue = false;
};

#endif
