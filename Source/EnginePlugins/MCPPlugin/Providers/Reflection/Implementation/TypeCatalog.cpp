/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/World/Component.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Reflection/Reflection.h>
#include <MCPPlugin/Json.h>
#include <MCPPlugin/Providers/RuntimeProviders.h>
#include <MCPPlugin/Reflection.h>

namespace
{
  xiiVariantDictionary DescribeReflectedType(const xiiRTTI& type)
  {
    xiiVariantDictionary entry;
    entry["name"]        = xiiString(type.GetTypeName());
    entry["plugin"]      = xiiString(type.GetPluginName());
    entry["version"]     = type.GetTypeVersion();
    entry["abstract"]    = type.GetTypeFlags().IsSet(xiiTypeFlags::Abstract);
    entry["allocatable"] = type.GetAllocator() != nullptr && type.GetAllocator()->CanAllocate();
    if (const auto* pBase = type.GetParentType())
    {
      entry["base"] = xiiString(pBase->GetTypeName());
    }
    return entry;
  }

  xiiStatus SearchReflectedTypes(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    xiiUInt64   uiOffset = 0;
    xiiUInt64   uiLimit  = 32;
    const auto* pOffset  = arguments.GetValue("offset");
    const auto* pLimit   = arguments.GetValue("limit");
    if ((pOffset != nullptr && !xiiMCPJson::ReadUInt(*pOffset, uiOffset)) ||
        (pLimit != nullptr && !xiiMCPJson::ReadUInt(*pLimit, uiLimit)) || uiOffset > xiiMath::MaxValue<xiiUInt32>() || uiLimit == 0 || uiLimit > 128)
    {
      return xiiStatus("Provide a nonnegative offset and a limit from 1 to 128.");
    }

    const auto  sName   = xiiMCPJson::GetString(arguments, "query");
    const auto  sPlugin = xiiMCPJson::GetString(arguments, "plugin");
    const auto  sBase   = xiiMCPJson::GetString(arguments, "base");
    const auto* pBase   = sBase.IsEmpty() ? nullptr : xiiRTTI::FindTypeByName(sBase);
    if (!sBase.IsEmpty() && pBase == nullptr)
    {
      return xiiStatus("The base type is not registered in this process.");
    }

    xiiVariantArray entries;
    xiiUInt64       uiMatched = 0;
    bool            bHasMore  = false;
    auto            collect   = [&](const xiiRTTI* pType) {
      if (bHasMore || context.HasExpired() || (!sName.IsEmpty() && !pType->GetTypeName().FindSubString_NoCase(sName)) ||
          (!sPlugin.IsEmpty() && pType->GetPluginName() != sPlugin))
      {
        return;
      }
      if (uiMatched++ >= uiOffset)
      {
        bHasMore = entries.GetCount() == uiLimit;
        if (!bHasMore)
        {
          entries.PushBack(DescribeReflectedType(*pType));
        }
      }
    };
    if (pBase != nullptr)
    {
      xiiRTTI::ForEachDerivedType(pBase, collect);
    }
    else
    {
      xiiRTTI::ForEachType(collect);
    }
    if (context.HasExpired())
    {
      return xiiStatus("Type discovery exceeded its deadline.");
    }
    out_result["types"] = entries;
    if (bHasMore)
    {
      out_result["nextOffset"] = uiOffset + uiLimit;
    }
    return XII_SUCCESS;
  }

  xiiStatus InspectReflectedType(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto* pType = xiiRTTI::FindTypeByName(xiiMCPJson::GetString(arguments, "type"));
    if (pType == nullptr)
    {
      return xiiStatus("The type is not registered in this process.");
    }
    out_result                = DescribeReflectedType(*pType);
    out_result["component"]   = pType->IsDerivedFrom<xiiComponent>();
    out_result["resource"]    = pType->IsDerivedFrom<xiiResource>();
    out_result["worldModule"] = pType->IsDerivedFrom<xiiWorldModule>();
    out_result["schema"]      = xiiMCPReflection::Describe(*pType);
    return XII_SUCCESS;
  }
} // namespace

void xiiMCPRuntimeProviders::RegisterReflection(xiiMCPRegistry& ref_registry)
{
  ref_registry.RegisterTool("runtime.types", "Find registered RTTI types by name, plugin or base type. Use xiiComponent or xiiResource to discover the corresponding domain.",
                            R"({"type":"object","properties":{"query":{"type":"string"},"plugin":{"type":"string"},"base":{"type":"string"},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":128}}})", xiiMCPPermission::Read, &SearchReflectedTypes)
    .IgnoreResult();
  ref_registry.RegisterTool("runtime.type", "Inspect an RTTI type, its base and generated schema for supported reflected properties.",
                            R"({"type":"object","required":["type"],"properties":{"type":{"type":"string"}}})", xiiMCPPermission::Read, &InspectReflectedType)
    .IgnoreResult();
}
