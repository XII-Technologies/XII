#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class XII_CORE_DLL xiiScriptExtensionClass_CVar
{
public:
  static xiiVariant GetValue(xiiStringView sName);
  static bool       GetBoolValue(xiiStringView sName);
  static int        GetIntValue(xiiStringView sName);
  static float      GetFloatValue(xiiStringView sName);
  static double     GetDoubleValue(xiiStringView sName);
  static xiiString  GetStringValue(xiiStringView sName);

  static void SetValue(xiiStringView sName, const xiiVariant& value);
  static void SetBoolValue(xiiStringView sName, bool bValue);
  static void SetIntValue(xiiStringView sName, int iValue);
  static void SetFloatValue(xiiStringView sName, float fValue);
  static void SetDoubleValue(xiiStringView sName, double fValue);
  static void SetStringValue(xiiStringView sName, const xiiString& sValue);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptExtensionClass_CVar);
