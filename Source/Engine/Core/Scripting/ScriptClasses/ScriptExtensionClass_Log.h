#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class XII_CORE_DLL xiiScriptExtensionClass_Log
{
public:
  static void Info(xiiStringView sText, const xiiVariantArray& params);
  static void Warning(xiiStringView sText, const xiiVariantArray& params);
  static void Error(xiiStringView sText, const xiiVariantArray& params);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptExtensionClass_Log);
