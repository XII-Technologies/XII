/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Log.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiScriptExtensionClass_Log, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(Info, In, "Text", In, "Params")->AddAttributes(new xiiDynamicPinAttribute("Params")),
    XII_SCRIPT_FUNCTION_PROPERTY(Warning, In, "Text", In, "Params")->AddAttributes(new xiiDynamicPinAttribute("Params")),
    XII_SCRIPT_FUNCTION_PROPERTY(Error, In, "Text", In, "Params")->AddAttributes(new xiiDynamicPinAttribute("Params")),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiScriptExtensionAttribute("Log"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

static xiiStringView BuildFormattedText(xiiStringView sText, const xiiVariantArray& params, xiiStringBuilder& ref_sStorage)
{
  xiiHybridArray<xiiString, 12> stringStorage;
  stringStorage.Reserve(params.GetCount());
  for (auto& param : params)
  {
    stringStorage.PushBack(param.ConvertTo<xiiString>());
  }

  xiiHybridArray<xiiStringView, 12> stringViews;
  stringViews.Reserve(stringStorage.GetCount());
  for (auto& s : stringStorage)
  {
    stringViews.PushBack(s);
  }

  xiiFormatString fs(sText);
  return fs.BuildFormattedText(ref_sStorage, stringViews.GetData(), stringViews.GetCount());
}

// static
void xiiScriptExtensionClass_Log::Info(xiiStringView sText, const xiiVariantArray& params)
{
  xiiStringBuilder sStorage;
  xiiLog::Info(BuildFormattedText(sText, params, sStorage));
}

// static
void xiiScriptExtensionClass_Log::Warning(xiiStringView sText, const xiiVariantArray& params)
{
  xiiStringBuilder sStorage;
  xiiLog::Warning(BuildFormattedText(sText, params, sStorage));
}

// static
void xiiScriptExtensionClass_Log::Error(xiiStringView sText, const xiiVariantArray& params)
{
  xiiStringBuilder sStorage;
  xiiLog::Error(BuildFormattedText(sText, params, sStorage));
}

XII_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_Log);
