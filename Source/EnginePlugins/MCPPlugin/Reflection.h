/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MCPPlugin/MCPPluginDLL.h>

#include <Foundation/Types/Status.h>
#include <Foundation/Types/Variant.h>

class xiiRTTI;
class xiiAbstractProperty;

namespace xiiMCPReflection
{
  [[nodiscard]] XII_MCPPLUGIN_DLL xiiVariantDictionary Describe(const xiiRTTI& type);
  [[nodiscard]] XII_MCPPLUGIN_DLL xiiVariantDictionary ReadMembers(const xiiRTTI& type, const void* pObject);
  [[nodiscard]] XII_MCPPLUGIN_DLL bool                 ConvertValue(const xiiVariant& value, xiiVariant& out_json);
  [[nodiscard]] XII_MCPPLUGIN_DLL bool                 ReadFloatArray(const xiiVariant& json, xiiArrayPtr<float> out_values);
  [[nodiscard]] XII_MCPPLUGIN_DLL bool                 SupportsArgument(const xiiAbstractProperty& property);
  [[nodiscard]] XII_MCPPLUGIN_DLL bool                 ConvertArgument(const xiiAbstractProperty& property, const xiiVariant& json, xiiVariant& out_value);
  [[nodiscard]] XII_MCPPLUGIN_DLL xiiStatus            SetRuntimeMember(const xiiRTTI& type, void* pObject, xiiStringView sProperty, const xiiVariant& json);
} // namespace xiiMCPReflection
