/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginMCP/EditorPluginMCPDLL.h>

#include <Foundation/Types/Uuid.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <MCPPlugin/Json.h>

namespace xiiEditorMCPArguments
{
  inline xiiString FormatGuid(const xiiUuid& guid)
  {
    xiiStringBuilder sGuid;
    xiiConversionUtils::ToString(guid, sGuid);
    return xiiString(sGuid);
  }

  inline xiiUuid ReadGuid(const xiiVariantDictionary& arguments, xiiStringView sKey)
  {
    const auto sValue = xiiMCPJson::GetString(arguments, sKey);
    return xiiConversionUtils::IsStringUuid(sValue) ? xiiConversionUtils::ConvertStringToUuid(sValue) : xiiUuid();
  }

  inline bool ReadUnsigned(const xiiVariantDictionary& arguments, xiiStringView sKey, xiiUInt64& out_uiValue)
  {
    const auto* pValue = arguments.GetValue(sKey);
    return pValue != nullptr && xiiMCPJson::ReadUInt(*pValue, out_uiValue);
  }

  inline bool ReadPage(const xiiVariantDictionary& arguments, xiiUInt64& out_uiOffset, xiiUInt64& out_uiLimit)
  {
    out_uiOffset = 0;
    out_uiLimit  = 32;
    return (!arguments.Contains("offset") || ReadUnsigned(arguments, "offset", out_uiOffset)) &&
      (!arguments.Contains("limit") || ReadUnsigned(arguments, "limit", out_uiLimit)) &&
      out_uiLimit > 0 && out_uiLimit <= 128;
  }
} // namespace xiiEditorMCPArguments
