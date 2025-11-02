#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

/// Script extension class providing logging functionality from scripts.
///
/// Allows scripts to output formatted log messages at different severity levels.
/// Messages are sent to the standard xiiEngine logging system and will appear in the console, log files, and other registered log writers.
class XII_CORE_DLL xiiScriptExtensionClass_Log
{
public:
  static void Info(xiiStringView sText, const xiiVariantArray& params);
  static void Warning(xiiStringView sText, const xiiVariantArray& params);
  static void Error(xiiStringView sText, const xiiVariantArray& params);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiScriptExtensionClass_Log);
