/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/Scripting/ScriptComponent.h>
#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <MCPPlugin/Json.h>
#include <MCPPlugin/Providers/RuntimeProviders.h>
#include <MCPPlugin/Providers/World/World.h>
#include <MCPPlugin/Reflection.h>

namespace
{
  using xiiMCPRuntimeProviders::FindObject;
  using xiiMCPRuntimeProviders::FindWorld;
  using xiiMCPRuntimeProviders::FormatHandle;

  constexpr xiiUInt32 MaximumVariableNameBytes = 256;
  constexpr xiiUInt32 MaximumStringValueBytes  = 8192;

  xiiScriptComponent* FindScript(xiiWorld& ref_world, const xiiVariantDictionary& arguments)
  {
    xiiUInt64 uiId    = 0;
    auto*     pObject = FindObject(ref_world, xiiMCPJson::GetString(arguments, "object"));
    if (pObject == nullptr || xiiConversionUtils::StringToUInt64Strict(xiiMCPJson::GetString(arguments, "component"), uiId).Failed())
    {
      return nullptr;
    }
    for (auto* pComponent : pObject->GetComponents())
    {
      if (pComponent->GetHandle().GetInternalID().m_Data == uiId)
      {
        return xiiDynamicCast<xiiScriptComponent*>(pComponent);
      }
    }
    return nullptr;
  }

  xiiStringView ScriptClass(const xiiScriptComponent& script)
  {
    const auto& resource = script.GetScriptClass();
    return resource.IsValid() ? resource.GetResourceID() : xiiStringView();
  }

  void DescribeScript(const xiiScriptComponent& script, xiiVariantDictionary& out_result)
  {
    out_result["id"]                       = FormatHandle(script.GetHandle());
    out_result["scriptClass"]              = xiiString(ScriptClass(script));
    out_result["active"]                   = script.IsActive();
    out_result["instantiated"]             = script.GetScriptInstance() != nullptr;
    out_result["updateIntervalSeconds"]    = script.GetUpdateInterval().GetSeconds();
    out_result["updateOnlyWhenSimulating"] = script.GetUpdateOnlyWhenSimulating();
  }

  xiiVariant ReadVariable(const xiiScriptComponent& script, xiiStringView sName)
  {
    xiiHashedString name;
    name.Assign(sName);
    return script.GetScriptVariable(name);
  }

  void DescribeVariable(const xiiVariant& value, xiiVariantDictionary& out_result)
  {
    if (const auto* pType = value.GetReflectedType())
    {
      out_result["type"] = xiiString(pType->GetTypeName());
    }
    out_result["writable"] = xiiMCPReflection::SupportsTypedValue(value.GetType());
  }

  bool WriteVariableValue(const xiiVariant& value, xiiVariantDictionary& out_result)
  {
    return (!value.IsA<xiiString>() || value.Get<xiiString>().GetElementCount() <= MaximumStringValueBytes) &&
      xiiMCPReflection::ConvertValue(value, out_result["value"]);
  }

  xiiStatus GetScript(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    auto* pWorld = FindWorld(arguments);
    if (pWorld == nullptr)
    {
      return xiiStatus("Select a current world.");
    }
    XII_LOCK(pWorld->GetReadMarker());
    const auto* pScript = FindScript(*pWorld, arguments);
    if (pScript == nullptr)
    {
      return xiiStatus("Select a live script component on the given object.");
    }
    DescribeScript(*pScript, out_result);

    if (const auto* pName = arguments.GetValue("variable"))
    {
      if (!pName->IsA<xiiString>() || pName->Get<xiiString>().IsEmpty() || pName->Get<xiiString>().GetElementCount() > MaximumVariableNameBytes)
      {
        return xiiStatus("Provide a script variable name of at most 256 UTF-8 bytes.");
      }
      const auto& sName = pName->Get<xiiString>();
      const auto  value = ReadVariable(*pScript, sName);
      if (!value.IsValid())
      {
        return xiiStatus("The script has no initialized value for this variable.");
      }
      xiiVariantDictionary variable;
      variable["name"] = sName;
      DescribeVariable(value, variable);
      if (!WriteVariableValue(value, variable))
      {
        return xiiStatus("This variable value cannot be represented by MCP or exceeds the value budget.");
      }
      out_result["variable"] = std::move(variable);
      return XII_SUCCESS;
    }

    const auto* pOffset  = arguments.GetValue("offset");
    const auto* pLimit   = arguments.GetValue("limit");
    xiiUInt64   uiOffset = 0;
    xiiUInt64   uiLimit  = 32;
    if ((pOffset != nullptr && (!xiiMCPJson::ReadUInt(*pOffset, uiOffset) || uiOffset > xiiMath::MaxValue<xiiUInt32>())) ||
        (pLimit != nullptr && (!xiiMCPJson::ReadUInt(*pLimit, uiLimit) || uiLimit < 1 || uiLimit > 64)))
    {
      return xiiStatus("Provide an offset and a page size from 1 to 64.");
    }
    xiiHybridArray<xiiString, 32> names;
    if (const auto* pInstance = pScript->GetScriptInstance())
    {
      pInstance->GetInstanceVariableNames(names);
      names.Sort();
    }
    const xiiUInt64 uiEnd = xiiMath::Min<xiiUInt64>(uiOffset + uiLimit, names.GetCount());
    xiiVariantArray variables;
    for (xiiUInt64 uiIndex = uiOffset; uiIndex < uiEnd; ++uiIndex)
    {
      const auto&          sName = names[static_cast<xiiUInt32>(uiIndex)];
      xiiVariantDictionary variable;
      variable["name"] = sName;
      DescribeVariable(ReadVariable(*pScript, sName), variable);
      variables.PushBack(std::move(variable));
    }
    out_result["variableCount"] = names.GetCount();
    out_result["variables"]     = std::move(variables);
    if (uiEnd < names.GetCount())
    {
      out_result["nextOffset"] = uiEnd;
    }
    return XII_SUCCESS;
  }

  xiiStatus ConfigureScript(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    auto*       pWorld    = FindWorld(arguments);
    const auto* pInterval = arguments.GetValue("updateIntervalSeconds");
    const auto* pOnly     = arguments.GetValue("updateOnlyWhenSimulating");
    if (pWorld == nullptr || (pInterval == nullptr && pOnly == nullptr) ||
        (pInterval != nullptr && (!pInterval->IsNumber() || pInterval->IsA<bool>() || !xiiMath::IsInRange(pInterval->ConvertTo<double>(), 0.0, 86400.0))) ||
        (pOnly != nullptr && !pOnly->IsA<bool>()))
    {
      return xiiStatus("Provide a live world and an interval from 0 to 86400 seconds or a boolean simulation-only state.");
    }
    XII_LOCK(pWorld->GetWriteMarker());
    auto* pScript = FindScript(*pWorld, arguments);
    if (pScript == nullptr || context.HasExpired())
    {
      return xiiStatus("The script component is unavailable or the request expired.");
    }
    if (pInterval != nullptr)
    {
      pScript->SetUpdateInterval(xiiTime::MakeFromSeconds(pInterval->ConvertTo<double>()));
    }
    if (pOnly != nullptr)
    {
      pScript->SetUpdateOnlyWhenSimulating(pOnly->Get<bool>());
    }
    DescribeScript(*pScript, out_result);
    return XII_SUCCESS;
  }

  xiiStatus SetScriptVariable(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    auto*       pWorld = FindWorld(arguments);
    const auto  sName  = xiiMCPJson::GetString(arguments, "variable");
    const auto* pValue = arguments.GetValue("value");
    if (pWorld == nullptr || sName.IsEmpty() || sName.GetElementCount() > MaximumVariableNameBytes || pValue == nullptr)
    {
      return xiiStatus("Provide a live world, a script variable name and a typed value.");
    }
    XII_LOCK(pWorld->GetWriteMarker());
    auto* pScript = FindScript(*pWorld, arguments);
    if (pScript == nullptr || pScript->GetScriptInstance() == nullptr || ScriptClass(*pScript) != xiiMCPJson::GetString(arguments, "scriptClass"))
    {
      return xiiStatus("The script instance or class changed. Inspect the component again.");
    }

    xiiHashedString name;
    name.Assign(sName);
    const auto current = pScript->GetScriptVariable(name);
    xiiVariant value;
    if (!xiiMCPReflection::ConvertTypedValue(current.GetType(), *pValue, value) || !WriteVariableValue(value, out_result))
    {
      return xiiStatus("The value must match the initialized variable's supported type and size.");
    }
    if (context.HasExpired())
    {
      return xiiStatus("The script edit expired.");
    }
    pScript->SetScriptVariable(name, value);
    if (pScript->GetScriptVariable(name) != value)
    {
      return xiiStatus("The script backend did not retain the requested value.");
    }
    out_result["name"] = xiiString(sName);
    DescribeVariable(value, out_result);
    return XII_SUCCESS;
  }

  xiiScriptClassResourceHandle FindOrLoadScriptClass(xiiStringView sClass)
  {
    auto resource = xiiResourceManager::GetExistingResource<xiiScriptClassResource>(sClass);
    if (resource.IsValid())
    {
      return resource;
    }

    // Resolve a compiled asset from the runtime's own asset table before requesting any I/O.
    const auto*      pType = xiiResourceManager::FindResourceForAssetType("VisualScriptClass");
    xiiStringBuilder sRedirectedPath;
    if (pType != nullptr && pType->IsDerivedFrom<xiiScriptClassResource>() && xiiConversionUtils::IsStringUuid(sClass) &&
        xiiFileSystem::ResolveAssetRedirection(sClass, sRedirectedPath) && sRedirectedPath.HasExtension("xiiBinVisualScriptClass"))
    {
      resource.AssignFromTypelessHandle(xiiResourceManager::LoadResourceByType(pType, sClass));
    }
    return resource;
  }

  xiiStatus SetScriptClass(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    auto*       pWorld   = FindWorld(arguments);
    const auto* pClassId = arguments.GetValue("scriptClass");
    if (pWorld == nullptr || pClassId == nullptr || (pClassId->IsValid() && !pClassId->IsA<xiiString>()))
    {
      return xiiStatus("Select a live world and provide a script class ID or null to clear the class.");
    }
    XII_LOCK(pWorld->GetWriteMarker());
    auto* pScript = FindScript(*pWorld, arguments);
    if (pScript == nullptr || context.HasExpired())
    {
      return xiiStatus("The script component is unavailable or the request expired.");
    }

    xiiScriptClassResourceHandle resource;
    if (pClassId->IsValid())
    {
      const auto& sClass = pClassId->Get<xiiString>();
      if (sClass.IsEmpty() || sClass.GetElementCount() > 512)
      {
        return xiiStatus("Provide a script resource ID from runtime.resources or runtime.script.");
      }
      resource = FindOrLoadScriptClass(sClass);
      if (!resource.IsValid())
      {
        return xiiStatus("The script resource is unavailable. Select a known component script resource or an indexed Visual Script asset UUID.");
      }
      const auto state = xiiResourceManager::GetLoadingState(resource);
      if (state == xiiResourceState::Unloaded)
      {
        xiiResourceManager::PreloadResource(resource);
        out_result["pending"]        = true;
        out_result["requestedClass"] = xiiString(resource.GetResourceID());
        return XII_SUCCESS;
      }
      xiiResourceLock<xiiScriptClassResource> loaded(resource, xiiResourceAcquireMode::PointerOnly);
      if (state != xiiResourceState::Loaded || loaded->GetType() == nullptr || !loaded->GetType()->IsDerivedFrom<xiiComponent>())
      {
        return xiiStatus("The selected resource is not a loaded component script class.");
      }
    }

    if (context.HasExpired())
    {
      return xiiStatus("The script class request expired.");
    }
    pScript->SetScriptClass(resource);
    if (const auto* pCurrent = FindScript(*pWorld, arguments))
    {
      DescribeScript(*pCurrent, out_result);
    }
    else
    {
      out_result["removed"] = true;
    }
    return XII_SUCCESS;
  }
} // namespace

void xiiMCPRuntimeProviders::RegisterScripting(xiiMCPRegistry& ref_registry)
{
  ref_registry.RegisterTool("runtime.script", "Inspect a live script component and page its variable names and types. Request one variable by name to read its value.",
                            R"({"type":"object","required":["world","object","component"],"properties":{"world":{"type":"string"},"object":{"type":"string"},"component":{"type":"string"},"variable":{"type":"string","minLength":1,"maxLength":256},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":64}}})",
                            xiiMCPPermission::Read, &GetScript)
    .IgnoreResult();
  ref_registry.RegisterTool("runtime.configure_script", "Set a script component's update interval or whether it updates only while simulating.",
                            R"({"type":"object","required":["world","object","component"],"properties":{"world":{"type":"string"},"object":{"type":"string"},"component":{"type":"string"},"updateIntervalSeconds":{"type":"number","minimum":0,"maximum":86400},"updateOnlyWhenSimulating":{"type":"boolean"}}})",
                            xiiMCPPermission::Edit, &ConfigureScript)
    .IgnoreResult();
  ref_registry.RegisterTool("runtime.set_script_variable", "Write a supported live script variable using its current native type. This changes the instance until it is reinitialized.",
                            R"({"type":"object","required":["world","object","component","scriptClass","variable","value"],"properties":{"world":{"type":"string"},"object":{"type":"string"},"component":{"type":"string"},"scriptClass":{"type":"string"},"variable":{"type":"string","minLength":1,"maxLength":256},"value":{}}})",
                            xiiMCPPermission::Edit, &SetScriptVariable)
    .IgnoreResult();
  ref_registry.RegisterTool("runtime.set_script_class", "Assign a known component script class or clear it with null. Indexed Visual Script asset UUIDs load in the background. Repeat the request if pending is true.",
                            R"({"type":"object","required":["world","object","component","scriptClass"],"properties":{"world":{"type":"string"},"object":{"type":"string"},"component":{"type":"string"},"scriptClass":{"type":["string","null"]}}})",
                            xiiMCPPermission::Execute, &SetScriptClass)
    .IgnoreResult();
}
