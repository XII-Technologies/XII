#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

namespace xiiRmlUiConversionUtils
{
  XII_RMLUIPLUGIN_DLL xiiVariant ToVariant(const Rml::Variant& value, xiiVariant::Type::Enum targetType = xiiVariant::Type::Invalid);
  XII_RMLUIPLUGIN_DLL Rml::Variant ToVariant(const xiiVariant& value);
} // namespace xiiRmlUiConversionUtils
