/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <MCPPlugin/Json.h>
#include <MCPPlugin/Providers/RuntimeProviders.h>

xiiStatus xiiMCPRuntimeProviders::WriteResource(xiiStringView sUri, const xiiVariantDictionary& data, xiiVariantDictionary& out_result)
{
  xiiStringBuilder sData;
  if (xiiMCPJson::Write(data, sData).Failed())
  {
    return xiiStatus("The resource snapshot exceeded its response budget.");
  }
  xiiVariantDictionary content;
  content["uri"]      = xiiString(sUri);
  content["mimeType"] = xiiString("application/json");
  content["text"]     = xiiString(sData);
  xiiVariantArray contents;
  contents.PushBack(std::move(content));
  out_result["contents"] = std::move(contents);
  return XII_SUCCESS;
}
