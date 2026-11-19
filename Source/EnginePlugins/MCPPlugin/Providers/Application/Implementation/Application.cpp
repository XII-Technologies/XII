/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/SubSystem.h>
#include <MCPPlugin/Json.h>
#include <MCPPlugin/Providers/RuntimeProviders.h>

namespace
{
  xiiStatus ListSubsystems(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto      sGroup = xiiMCPJson::GetString(arguments, "group");
    const auto      sQuery = xiiMCPJson::GetString(arguments, "query");
    xiiVariantArray rows;
    for (auto* pSubsystem = xiiSubSystem::GetFirstInstance(); pSubsystem != nullptr; pSubsystem = pSubsystem->GetNextInstance())
    {
      if (context.HasExpired())
      {
        return xiiStatus("Subsystem discovery exceeded its deadline.");
      }
      if ((!sGroup.IsEmpty() && pSubsystem->GetGroupName() != sGroup) ||
          (!sQuery.IsEmpty() && pSubsystem->GetSubSystemName().FindSubString_NoCase(sQuery) == nullptr))
      {
        continue;
      }
      if (rows.GetCount() == 128)
      {
        out_result["truncated"] = true;
        break;
      }

      xiiVariantDictionary row;
      row["name"]             = xiiString(pSubsystem->GetSubSystemName());
      row["group"]            = xiiString(pSubsystem->GetGroupName());
      row["plugin"]           = xiiString(pSubsystem->GetPluginName());
      row["coreStarted"]      = pSubsystem->IsStartupPhaseDone(xiiStartupStage::CoreSystems);
      row["highLevelStarted"] = pSubsystem->IsStartupPhaseDone(xiiStartupStage::HighLevelSystems);
      xiiVariantArray dependencies;
      for (xiiInt32 iIndex = 0; iIndex < 64; ++iIndex)
      {
        const auto sDependency = pSubsystem->GetDependency(iIndex);
        if (sDependency.IsEmpty())
        {
          break;
        }
        dependencies.PushBack(xiiString(sDependency));
      }
      row["dependencies"] = dependencies;
      rows.PushBack(row);
    }
    out_result["subsystems"] = rows;
    return XII_SUCCESS;
  }

  xiiStatus InspectRuntime(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result)
  {
    if (const auto* pApplication = xiiGameApplicationBase::GetGameApplicationBaseInstance())
    {
      out_result["application"]  = pApplication->GetApplicationName();
      out_result["frameSeconds"] = pApplication->GetFrameTime().GetSeconds();
    }
    out_result["worldSlots"] = xiiWorld::GetWorldCount();
    return XII_SUCCESS;
  }

  xiiStatus ReadRuntimeStatus(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    xiiVariantDictionary data;
    const auto           status = InspectRuntime(context, arguments, data);
    return status.Succeeded() ? xiiMCPRuntimeProviders::WriteResource(xiiMCPJson::GetString(arguments, "uri"), data, out_result) : status;
  }
} // namespace

void xiiMCPRuntimeProviders::RegisterApplication(xiiMCPRegistry& ref_registry)
{
  const auto read = xiiMCPPermission::Read;
  ref_registry.RegisterTool("runtime.subsystems", "List subsystem startup phases, owning plugins and dependencies. Filter by group or subsystem name to narrow results.",
                            R"({"type":"object","properties":{"group":{"type":"string"},"query":{"type":"string"}}})", read, &ListSubsystems)
    .IgnoreResult();
  ref_registry.RegisterTool("runtime.status", "Report application frame time and live world registry size.", R"({"type":"object","additionalProperties":false})", read, &InspectRuntime).IgnoreResult();

  xiiVariantDictionary status;
  status["name"]        = xiiString("Runtime status");
  status["description"] = xiiString("Application name, frame time and current world registry size.");
  status["mimeType"]    = xiiString("application/json");
  ref_registry.Register(xiiMCPCapability::Resource, "xii://runtime/status", std::move(status), read, &ReadRuntimeStatus).IgnoreResult();
}
