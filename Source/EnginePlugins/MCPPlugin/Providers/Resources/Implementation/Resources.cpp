/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <MCPPlugin/Json.h>
#include <MCPPlugin/Providers/RuntimeProviders.h>

namespace
{
  xiiStatus ListResources(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto  sType = xiiMCPJson::GetString(arguments, "type");
    const auto* pType = xiiRTTI::FindTypeByName(sType);
    if (pType == nullptr || !pType->IsDerivedFrom<xiiResource>())
    {
      return xiiStatus("Provide a registered resource RTTI type.");
    }

    xiiUInt64   uiOffset = 0;
    xiiUInt64   uiLimit  = 32;
    const auto* pOffset  = arguments.GetValue("offset");
    const auto* pLimit   = arguments.GetValue("limit");
    if ((pOffset != nullptr && !xiiMCPJson::ReadUInt(*pOffset, uiOffset)) || (pLimit != nullptr && !xiiMCPJson::ReadUInt(*pLimit, uiLimit)) ||
        uiOffset > xiiMath::MaxValue<xiiUInt32>() || uiLimit == 0 || uiLimit > 128)
    {
      return xiiStatus("Provide a nonnegative offset and a limit from 1 to 128.");
    }

    out_result["loadingInProgress"] = xiiResourceManager::IsAnyLoadingInProgress();
    const auto      sQuery          = xiiMCPJson::GetString(arguments, "query");
    auto            resources       = xiiResourceManager::GetAllResourcesOfType(*pType, static_cast<xiiUInt32>(uiLimit + 1), static_cast<xiiUInt32>(uiOffset), sQuery);
    xiiVariantArray rows;
    for (const auto* pResource : *resources)
    {
      if (rows.GetCount() >= uiLimit)
      {
        out_result["nextOffset"] = uiOffset + uiLimit;
        break;
      }
      xiiVariantDictionary row;
      row["id"]   = xiiString(pResource->GetResourceID());
      row["type"] = xiiString(pResource->GetDynamicRTTI()->GetTypeName());
      xiiResource::ContentStats stats;
      if (pResource->TryCaptureContentStats(stats))
      {
        switch (stats.m_State)
        {
          case xiiResourceState::Invalid:
            row["state"] = xiiString("invalid");
            break;
          case xiiResourceState::Unloaded:
            row["state"] = xiiString("unloaded");
            break;
          case xiiResourceState::LoadedResourceMissing:
            row["state"] = xiiString("missing");
            break;
          case xiiResourceState::Loaded:
            row["state"] = xiiString("loaded");
            break;
        }
        row["cpuBytes"]                 = stats.m_Memory.m_uiMemoryCPU;
        row["gpuBytes"]                 = stats.m_Memory.m_uiMemoryGPU;
        row["description"]              = xiiString(stats.m_sDescription.GetView().GetSubString(0, 300));
        row["loadableQualityLevels"]    = stats.m_uiLoadable;
        row["discardableQualityLevels"] = stats.m_uiDiscardable;
      }
      rows.PushBack(std::move(row));
    }
    out_result["resources"] = std::move(rows);
    return XII_SUCCESS;
  }

  xiiStatus ReloadResource(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto  sId   = xiiMCPJson::GetString(arguments, "id");
    const auto* pType = xiiRTTI::FindTypeByName(xiiMCPJson::GetString(arguments, "type"));
    if (sId.IsEmpty() || sId.GetElementCount() > 2048 || pType == nullptr || !pType->IsDerivedFrom<xiiResource>())
    {
      return xiiStatus("Provide an existing resource ID and its concrete RTTI type.");
    }

    const auto hResource = xiiResourceManager::GetExistingResourceByType(pType, sId);
    if (!hResource.IsValid())
    {
      return xiiStatus("The resource is no longer registered. Inspect resources again.");
    }
    out_result["reloadRequested"] = xiiResourceManager::ReloadResource(pType, hResource, false);
    return XII_SUCCESS;
  }
} // namespace

void xiiMCPRuntimeProviders::RegisterResources(xiiMCPRegistry& ref_registry)
{
  const auto read    = xiiMCPPermission::Read;
  const auto execute = xiiMCPPermission::Execute;
  ref_registry.RegisterTool("runtime.resources", "Page through registered resources by reflected type and ID, including asynchronous loading state, quality levels and memory use.", R"({"type":"object","required":["type"],"properties":{"type":{"type":"string"},"query":{"type":"string"},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":128}}})", read, &ListResources).IgnoreResult();
  ref_registry.RegisterTool("runtime.reload_resource", "Request a standard, non-forced reload of an existing resource. A false result means no reload was needed or available.",
                            R"({"type":"object","required":["id","type"],"properties":{"id":{"type":"string","maxLength":2048},"type":{"type":"string"}}})", execute, &ReloadResource)
    .IgnoreResult();
}
