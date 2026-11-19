/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <MCPPlugin/Implementation/RuntimeDiagnostics.h>
#include <MCPPlugin/Json.h>
#include <MCPPlugin/Reflection.h>
#include <MCPPlugin/Runtime.h>

#include <charconv>
#include <cmath>

namespace
{
  constexpr xiiUInt32 DefaultPageSize = 32;
  constexpr xiiUInt32 MaximumPageSize = 128;

  xiiString WorldId(const xiiWorld& world)
  {
    xiiStringBuilder sId;
    xiiConversionUtils::ToString(world.GetInstanceId(), sId);
    return xiiString(sId);
  }

  xiiWorld* FindWorld(const xiiVariantDictionary& arguments)
  {
    const auto sWorld = xiiMCPJson::GetString(arguments, "world");
    if (!xiiConversionUtils::IsStringUuid(sWorld))
    {
      return nullptr;
    }

    const auto instance = xiiConversionUtils::ConvertStringToUuid(sWorld);
    for (xiiUInt32 uiIndex = 0; uiIndex < xiiWorld::GetWorldCount(); ++uiIndex)
    {
      if (auto* pWorld = xiiWorld::GetWorld(uiIndex); pWorld != nullptr && pWorld->GetInstanceId() == instance)
      {
        return pWorld;
      }
    }
    return nullptr;
  }

  xiiString ObjectId(xiiGameObjectHandle hObject)
  {
    xiiStringBuilder sId;
    sId.SetFormat("{}", hObject.GetInternalID().m_Data);
    return xiiString(sId);
  }

  xiiGameObject* FindObject(xiiWorld& ref_world, xiiStringView sObject)
  {
    if (sObject.IsEmpty())
    {
      return nullptr;
    }

    xiiGameObjectId id;
    const auto      result = std::from_chars(sObject.GetStartPointer(), sObject.GetEndPointer(), id.m_Data);
    if (result.ec != std::errc() || result.ptr != sObject.GetEndPointer() || id.m_WorldIndex != ref_world.GetIndex())
    {
      return nullptr;
    }
    xiiGameObject* pObject = nullptr;
    return ref_world.TryGetObject(xiiGameObjectHandle(id), pObject) ? pObject : nullptr;
  }

  xiiVariantDictionary DescribeObject(const xiiGameObject& object)
  {
    xiiVariantDictionary result;
    result["id"]             = ObjectId(object.GetHandle());
    result["name"]           = xiiString(object.GetName());
    result["active"]         = object.IsActive();
    result["componentCount"] = object.GetComponents().GetCount();
    if (const auto* pParent = object.GetParent())
    {
      result["parent"] = ObjectId(pParent->GetHandle());
    }
    return result;
  }

  xiiVariantDictionary DescribeTransform(const xiiTransform& transform)
  {
    xiiVariantDictionary result;
    XII_VERIFY(xiiMCPReflection::ConvertValue(transform.m_vPosition, result["position"]), "Position must be JSON-compatible.");
    XII_VERIFY(xiiMCPReflection::ConvertValue(transform.m_qRotation, result["rotation"]), "Rotation must be JSON-compatible.");
    XII_VERIFY(xiiMCPReflection::ConvertValue(transform.m_vScale, result["scale"]), "Scale must be JSON-compatible.");
    return result;
  }

  void DescribeSimulation(const xiiWorld& world, xiiVariantDictionary& out_result)
  {
    const auto& clock               = world.GetClock();
    out_result["simulationEnabled"] = world.GetWorldSimulationEnabled();
    out_result["paused"]            = clock.GetPaused();
    out_result["speed"]             = clock.GetSpeed();
    out_result["timeSeconds"]       = clock.GetAccumulatedTime().GetSeconds();
    out_result["stepSeconds"]       = clock.GetTimeDiff().GetSeconds();
    out_result["updateCount"]       = world.GetUpdateCounter();
  }

  bool ReadPage(const xiiVariantDictionary& arguments, xiiUInt64& out_uiOffset, xiiUInt64& out_uiLimit)
  {
    const auto* pOffset = arguments.GetValue("offset");
    const auto* pLimit  = arguments.GetValue("limit");
    out_uiOffset        = 0;
    out_uiLimit         = DefaultPageSize;
    return (pOffset == nullptr || xiiMCPJson::ReadUInt(*pOffset, out_uiOffset)) &&
      (pLimit == nullptr || xiiMCPJson::ReadUInt(*pLimit, out_uiLimit)) &&
      out_uiOffset <= xiiMath::MaxValue<xiiUInt32>() && out_uiLimit >= 1 && out_uiLimit <= MaximumPageSize;
  }

  xiiStatus ListWorlds(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result)
  {
    xiiVariantArray worlds;
    for (xiiUInt32 uiIndex = 0; uiIndex < xiiWorld::GetWorldCount(); ++uiIndex)
    {
      if (const auto* pWorld = xiiWorld::GetWorld(uiIndex))
      {
        XII_LOCK(pWorld->GetReadMarker());
        xiiVariantDictionary world;
        world["id"]          = WorldId(*pWorld);
        world["slot"]        = uiIndex;
        world["name"]        = xiiString(pWorld->GetName());
        world["objectCount"] = pWorld->GetObjectCount();
        DescribeSimulation(*pWorld, world);
        worlds.PushBack(std::move(world));
      }
    }
    out_result["worlds"] = std::move(worlds);
    return XII_SUCCESS;
  }

  xiiStatus ListObjects(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto* pWorld   = FindWorld(arguments);
    xiiUInt64   uiOffset = 0, uiLimit = 0;
    if (pWorld == nullptr || !ReadPage(arguments, uiOffset, uiLimit))
    {
      return xiiStatus("Provide a world ID from runtime.worlds and a valid page range.");
    }

    const auto sName   = xiiMCPJson::GetString(arguments, "name");
    const auto sType   = xiiMCPJson::GetString(arguments, "componentType");
    const auto sParent = xiiMCPJson::GetString(arguments, "parent");
    XII_LOCK(pWorld->GetReadMarker());
    xiiVariantArray objects;
    xiiUInt64       uiMatched = 0;
    xiiUInt32       uiVisited = 0;
    for (auto it = pWorld->GetObjects(); it.IsValid(); ++it)
    {
      if ((uiVisited++ % 128) == 0 && context.HasExpired())
      {
        return xiiStatus("The inspection deadline expired.");
      }
      if (!sName.IsEmpty() && it->GetName().FindSubString_NoCase(sName) == nullptr)
      {
        continue;
      }
      if (!sParent.IsEmpty() && (it->GetParent() == nullptr || ObjectId(it->GetParent()->GetHandle()) != sParent))
      {
        continue;
      }
      if (!sType.IsEmpty())
      {
        bool bHasType = false;
        for (const auto* pComponent : it->GetComponents())
        {
          if (pComponent->GetDynamicRTTI()->GetTypeName() == sType)
          {
            bHasType = true;
            break;
          }
        }
        if (!bHasType)
        {
          continue;
        }
      }
      if (uiMatched++ < uiOffset)
      {
        continue;
      }
      if (objects.GetCount() == uiLimit)
      {
        out_result["nextOffset"] = uiOffset + uiLimit;
        break;
      }
      objects.PushBack(DescribeObject(*it));
    }
    out_result["objects"] = std::move(objects);
    out_result["world"]   = WorldId(*pWorld);
    return XII_SUCCESS;
  }

  xiiStatus InspectObject(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    auto* pWorld = FindWorld(arguments);
    if (pWorld == nullptr)
    {
      return xiiStatus("The world is no longer available. List worlds again.");
    }

    xiiUInt64 uiOffset = 0, uiLimit = 0;
    if (!ReadPage(arguments, uiOffset, uiLimit) || uiLimit > 32)
    {
      return xiiStatus("The component page limit must be between 1 and 32.");
    }

    XII_LOCK(pWorld->GetReadMarker());
    const auto* pObject = FindObject(*pWorld, xiiMCPJson::GetString(arguments, "object"));
    if (pObject == nullptr)
    {
      return xiiStatus("The object no longer exists in the selected world.");
    }

    out_result                    = DescribeObject(*pObject);
    out_result["localTransform"]  = DescribeTransform(pObject->GetLocalTransform());
    out_result["globalTransform"] = DescribeTransform(pObject->GetGlobalTransform());
    xiiVariantArray components;
    xiiUInt64       uiIndex = 0;
    for (const auto* pComponent : pObject->GetComponents())
    {
      if (uiIndex++ < uiOffset)
      {
        continue;
      }
      if (components.GetCount() >= uiLimit)
      {
        out_result["nextOffset"] = uiOffset + uiLimit;
        break;
      }
      xiiVariantDictionary component;
      component["type"]   = xiiString(pComponent->GetDynamicRTTI()->GetTypeName());
      component["active"] = pComponent->IsActive();
      component["index"]  = uiIndex - 1;
      components.PushBack(std::move(component));
    }
    out_result["components"] = std::move(components);
    return XII_SUCCESS;
  }

  xiiStatus InspectComponent(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    auto*       pWorld  = FindWorld(arguments);
    xiiUInt64   uiIndex = 0;
    const auto* pIndex  = arguments.GetValue("index");
    if (pWorld == nullptr || pIndex == nullptr || !xiiMCPJson::ReadUInt(*pIndex, uiIndex))
    {
      return xiiStatus("Provide a world ID and component index from runtime.object.");
    }

    XII_LOCK(pWorld->GetReadMarker());
    const auto* pObject = FindObject(*pWorld, xiiMCPJson::GetString(arguments, "object"));
    if (pObject == nullptr || uiIndex >= pObject->GetComponents().GetCount())
    {
      return xiiStatus("The component is unavailable. Inspect the object again.");
    }

    const auto* pComponent    = pObject->GetComponents()[static_cast<xiiUInt32>(uiIndex)];
    const auto& type          = *pComponent->GetDynamicRTTI();
    const auto  sExpectedType = xiiMCPJson::GetString(arguments, "type");
    if (type.GetTypeName() != sExpectedType)
    {
      return xiiStatus("The component changed. Inspect the object again.");
    }
    out_result["schema"]     = xiiMCPReflection::Describe(type);
    out_result["properties"] = xiiMCPReflection::ReadMembers(type, pComponent);
    return XII_SUCCESS;
  }

  xiiStatus SetSimulation(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    auto*       pWorld   = FindWorld(arguments);
    const auto* pEnabled = arguments.GetValue("enabled");
    const auto* pPaused  = arguments.GetValue("paused");
    const auto* pSpeed   = arguments.GetValue("speed");
    if (pWorld == nullptr || (pEnabled != nullptr && !pEnabled->IsA<bool>()) || (pPaused != nullptr && !pPaused->IsA<bool>()) ||
        (pSpeed != nullptr && (!pSpeed->IsNumber() || pSpeed->IsA<bool>() || !xiiMath::IsInRange(pSpeed->ConvertTo<double>(), 0.0, 16.0))))
    {
      return xiiStatus("Provide a current world ID, boolean state fields and a speed between 0 and 16.");
    }

    XII_LOCK(pWorld->GetWriteMarker());
    if (pEnabled != nullptr)
    {
      pWorld->SetWorldSimulationEnabled(pEnabled->Get<bool>());
    }
    if (pPaused != nullptr)
    {
      pWorld->GetClock().SetPaused(pPaused->Get<bool>());
    }
    if (pSpeed != nullptr)
    {
      pWorld->GetClock().SetSpeed(pSpeed->ConvertTo<double>());
    }

    DescribeSimulation(*pWorld, out_result);
    return XII_SUCCESS;
  }

  bool ReadPosition(const xiiVariant& json, xiiVec3& out_vPosition)
  {
    float fValues[3] = {};
    if (!xiiMCPReflection::ReadFloatArray(json, fValues))
    {
      return false;
    }

    out_vPosition = xiiVec3(fValues[0], fValues[1], fValues[2]);
    return true;
  }

  xiiStatus EditObject(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    auto*       pWorld       = FindWorld(arguments);
    const auto  sAction      = xiiMCPJson::GetString(arguments, "action");
    const auto  sName        = xiiMCPJson::GetString(arguments, "name");
    const auto* pPosition    = arguments.GetValue("position");
    const auto* pRotation    = arguments.GetValue("rotation");
    const auto* pScale       = arguments.GetValue("scale");
    const auto* pActive      = arguments.GetValue("active");
    xiiVec3     vPosition    = xiiVec3::MakeZero();
    xiiVec3     vScale       = xiiVec3(1.0f);
    float       fRotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    if (pWorld == nullptr || sName.GetElementCount() > 128 || (pPosition != nullptr && !ReadPosition(*pPosition, vPosition)) ||
        (pScale != nullptr && (!ReadPosition(*pScale, vScale) || vScale.x == 0.0f || vScale.y == 0.0f || vScale.z == 0.0f)) ||
        (pRotation != nullptr && !xiiMCPReflection::ReadFloatArray(*pRotation, fRotation)) ||
        (pActive != nullptr && !pActive->IsA<bool>()))
    {
      return xiiStatus("Provide a current world ID and valid name, transform and active fields.");
    }
    const auto qRotation = xiiQuat::MakeFromElements(fRotation[0], fRotation[1], fRotation[2], fRotation[3]);
    if (!qRotation.IsValid())
    {
      return xiiStatus("The rotation must be a normalized quaternion using x, y, z, w order.");
    }

    XII_LOCK(pWorld->GetWriteMarker());
    xiiGameObject* pObject = nullptr;
    if (sAction == "create")
    {
      xiiGameObjectDescription description;
      pWorld->CreateObject(description, pObject);
    }
    else if (sAction == "modify" || sAction == "remove")
    {
      pObject = FindObject(*pWorld, xiiMCPJson::GetString(arguments, "object"));
    }
    if (pObject == nullptr)
    {
      return xiiStatus("The action or object is unavailable. Inspect the current world.");
    }

    if (sAction == "remove")
    {
      out_result["id"] = ObjectId(pObject->GetHandle());
      pWorld->DeleteObjectDelayed(pObject->GetHandle(), false);
      out_result["removalScheduled"] = true;
      return XII_SUCCESS;
    }
    if (!sName.IsEmpty())
    {
      pObject->SetName(sName);
    }
    if (pPosition != nullptr)
    {
      pObject->SetLocalPosition(vPosition);
    }
    if (pRotation != nullptr)
    {
      pObject->SetLocalRotation(qRotation);
    }
    if (pScale != nullptr)
    {
      pObject->SetLocalScaling(vScale);
    }
    if (pActive != nullptr)
    {
      pObject->SetActiveFlag(pActive->Get<bool>());
    }

    out_result                   = DescribeObject(*pObject);
    out_result["localTransform"] = DescribeTransform(pObject->GetLocalTransform());
    return XII_SUCCESS;
  }

  xiiStatus ListResources(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto  sType = xiiMCPJson::GetString(arguments, "type");
    const auto* pType = xiiRTTI::FindTypeByName(sType);
    if (pType == nullptr || !pType->IsDerivedFrom<xiiResource>())
    {
      return xiiStatus("Provide a registered resource RTTI type.");
    }

    // Existing resource enumeration takes a manager lock. Snapshot only one bounded prefix.
    auto            resources = xiiResourceManager::GetAllResourcesOfType<xiiResource>(256);
    xiiVariantArray rows;
    for (const auto* pResource : *resources)
    {
      if (!pResource->GetDynamicRTTI()->IsDerivedFrom(pType))
      {
        continue;
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
        row["cpuBytes"] = stats.m_Memory.m_uiMemoryCPU;
        row["gpuBytes"] = stats.m_Memory.m_uiMemoryGPU;
      }
      rows.PushBack(std::move(row));
    }
    out_result["resources"]     = std::move(rows);
    out_result["prefixLimited"] = resources->GetCount() == 256;
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

  xiiStatus WriteRuntimeResource(xiiStringView sUri, const xiiVariantDictionary& data, xiiVariantDictionary& out_result)
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

  xiiStatus ReadRuntimeStatus(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    xiiVariantDictionary data;
    const auto           status = InspectRuntime(context, arguments, data);
    return status.Succeeded() ? WriteRuntimeResource(xiiMCPJson::GetString(arguments, "uri"), data, out_result) : status;
  }

  xiiStatus ReadWorldSlot(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto sIndex  = xiiMCPJson::GetString(arguments, "index");
    xiiUInt32  uiIndex = 0;
    const auto result  = std::from_chars(sIndex.GetStartPointer(), sIndex.GetEndPointer(), uiIndex);
    if (result.ec != std::errc() || result.ptr != sIndex.GetEndPointer() || uiIndex >= xiiWorld::GetWorldCount())
    {
      return xiiStatus("The specified world slot is unavailable.");
    }
    const auto* pWorld = xiiWorld::GetWorld(uiIndex);
    if (pWorld == nullptr)
    {
      return xiiStatus("The specified world slot is empty.");
    }
    XII_LOCK(pWorld->GetReadMarker());
    xiiVariantDictionary data;
    data["id"]          = WorldId(*pWorld);
    data["name"]        = xiiString(pWorld->GetName());
    data["objectCount"] = pWorld->GetObjectCount();
    DescribeSimulation(*pWorld, data);
    return WriteRuntimeResource(xiiMCPJson::GetString(arguments, "uri"), data, out_result);
  }

  xiiStatus SceneInspectionPrompt(const xiiMCPCallContext&, const xiiVariantDictionary&, xiiVariantDictionary& out_result)
  {
    xiiVariantDictionary text;
    text["type"] = xiiString("text");
    text["text"] = xiiString("Inspect a live XII scene. Select a specific target, list its worlds, page through the chosen world's objects, "
                             "and inspect one object's transforms and reflected components. Report its simulation timing and any unavailable properties.");
    xiiVariantDictionary message;
    message["role"]    = xiiString("user");
    message["content"] = std::move(text);
    xiiVariantArray messages;
    messages.PushBack(std::move(message));
    out_result["messages"] = std::move(messages);
    return XII_SUCCESS;
  }

  void RegisterRuntimeTools(xiiMCPRegistry& ref_registry)
  {
    const auto read    = xiiMCPPermission::Read;
    const auto edit    = xiiMCPPermission::Edit;
    const auto execute = xiiMCPPermission::Execute;
    ref_registry.RegisterTool("runtime.worlds", "List live worlds and simulation timing. World IDs are valid for the lifetime of each world.", R"({"type":"object","additionalProperties":false})", read, &ListWorlds).IgnoreResult();
    ref_registry.RegisterTool("runtime.objects", "List live game objects using a bounded page and optional filters. Object IDs belong to the specified world.", R"({"type":"object","required":["world"],"properties":{"world":{"type":"string"},"name":{"type":"string"},"parent":{"type":"string"},"componentType":{"type":"string"},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":128,"default":32}}})", read, &ListObjects).IgnoreResult();
    ref_registry.RegisterTool("runtime.object", "Inspect one live game object, its local and global transforms, and a page of components.", R"({"type":"object","required":["world","object"],"properties":{"world":{"type":"string"},"object":{"type":"string"},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":32,"default":32}}})", read, &InspectObject).IgnoreResult();
    ref_registry.RegisterTool("runtime.component", "Inspect supported reflected properties on a live component. Discover its index and type with runtime.object.", R"({"type":"object","required":["world","object","index","type"],"properties":{"world":{"type":"string"},"object":{"type":"string"},"index":{"type":"integer","minimum":0},"type":{"type":"string"}}})", read, &InspectComponent).IgnoreResult();
    ref_registry.RegisterTool("runtime.edit_object", "Create, modify or schedule removal of a live object and its children. Transforms are local. Rotation is a normalized quaternion in x,y,z,w order.", R"({"type":"object","required":["world","action"],"properties":{"world":{"type":"string"},"action":{"enum":["create","modify","remove"]},"object":{"type":"string"},"name":{"type":"string","maxLength":128},"position":{"type":"array","items":{"type":"number"},"minItems":3,"maxItems":3},"rotation":{"type":"array","items":{"type":"number"},"minItems":4,"maxItems":4},"scale":{"type":"array","items":{"type":"number"},"minItems":3,"maxItems":3},"active":{"type":"boolean"}}})", edit, &EditObject).IgnoreResult();
    ref_registry.RegisterTool("runtime.simulation", "Set simulation enabled, clock pause, or simulation speed for a live world and return its current timing.", R"({"type":"object","required":["world"],"properties":{"world":{"type":"string"},"enabled":{"type":"boolean"},"paused":{"type":"boolean"},"speed":{"type":"number","minimum":0,"maximum":16}}})", execute, &SetSimulation).IgnoreResult();
    ref_registry.RegisterTool("runtime.resources", "Inspect matching resources from a bounded live resource snapshot, including async content state and memory use.", R"({"type":"object","required":["type"],"properties":{"type":{"type":"string"}}})", read, &ListResources).IgnoreResult();
    ref_registry.RegisterTool("runtime.reload_resource", "Request a standard, non-forced reload of an existing resource. A false result means no reload was needed or available.",
                              R"({"type":"object","required":["id","type"],"properties":{"id":{"type":"string","maxLength":2048},"type":{"type":"string"}}})", execute, &ReloadResource)
      .IgnoreResult();
    ref_registry.RegisterTool("runtime.status", "Report application frame time and live world registry size.", R"({"type":"object","additionalProperties":false})", read, &InspectRuntime).IgnoreResult();

    xiiVariantDictionary status;
    status["name"]        = xiiString("Runtime status");
    status["description"] = xiiString("Application name, frame time and current world registry size.");
    status["mimeType"]    = xiiString("application/json");
    ref_registry.Register(xiiMCPCapability::Resource, "xii://runtime/status", std::move(status), read, &ReadRuntimeStatus).IgnoreResult();

    xiiVariantDictionary worldSlot;
    worldSlot["name"]        = xiiString("Live world slot");
    worldSlot["description"] = xiiString("Snapshot of the world currently occupying a numeric registry slot. Use its returned ID for operations.");
    worldSlot["mimeType"]    = xiiString("application/json");
    ref_registry.Register(xiiMCPCapability::ResourceTemplate, "xii://runtime/worlds/{index}", std::move(worldSlot), read, &ReadWorldSlot).IgnoreResult();

    xiiVariantDictionary prompt;
    prompt["description"] = xiiString("Walk through inspection of a live world, its object hierarchy and reflected components.");
    ref_registry.Register(xiiMCPCapability::Prompt, "runtime.inspect_scene", std::move(prompt), read, &SceneInspectionPrompt).IgnoreResult();
  }

  xiiMCPRuntime* s_pRuntime = nullptr;

  void UpdateRuntime(const xiiGameApplicationExecutionEvent& event)
  {
    if (event.m_Type == xiiGameApplicationExecutionEvent::Type::AfterWorldUpdates && s_pRuntime != nullptr)
    {
      s_pRuntime->Update();
    }
  }

  void StartRuntime()
  {
    auto*       pApplication = xiiGameApplicationBase::GetGameApplicationBaseInstance();
    const auto* pCommandLine = xiiCommandLineUtils::GetGlobalInstance();
    const auto  sListen      = pCommandLine->GetStringOption("-MCPListen");
    if (pApplication == nullptr || (!pCommandLine->GetBoolOption("-MCP") && sListen.IsEmpty()))
    {
      return;
    }

    xiiMCPRuntimeConfiguration configuration;
    configuration.m_sApplication   = pApplication->GetApplicationName();
    configuration.m_sLocalEndpoint = pCommandLine->GetStringOption("-MCPLocal");
    configuration.m_bNative        = pCommandLine->GetBoolOption("-MCP");
    configuration.m_bAdvertise     = !pCommandLine->GetBoolOption("-MCPNoDiscovery");
    if (pCommandLine->GetBoolOption("-MCPAllowEdit"))
    {
      configuration.m_uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Edit);
    }
    if (pCommandLine->GetBoolOption("-MCPAllowCapture"))
    {
      configuration.m_uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Capture);
    }
    if (pCommandLine->GetBoolOption("-MCPAllowExecute"))
    {
      configuration.m_uiPermissions |= static_cast<xiiUInt8>(xiiMCPPermission::Execute);
    }
    if (!sListen.IsEmpty() && xiiNetworkEndpoint::Parse(sListen, configuration.m_Http.m_Endpoint).Failed())
    {
      xiiLog::Error("Invalid -MCPListen endpoint.");
      return;
    }
    configuration.m_Http.m_sBearerToken  = xiiEnvironmentVariableUtils::GetValueString("XII_MCP_TOKEN");
    configuration.m_Http.m_uiPermissions = configuration.m_uiPermissions;

    xiiUniquePtr<xiiMCPRuntime> pRuntime = XII_DEFAULT_NEW(xiiMCPRuntime);
    const auto                  status   = pRuntime->Start(configuration);
    if (status.Failed())
    {
      xiiLog::Error("Could not start MCP: {}", status.GetMessageString());
      return;
    }

    s_pRuntime = pRuntime.Release();
    pApplication->m_ExecutionEvents.AddEventHandler(&UpdateRuntime);
  }

  void StopRuntime()
  {
    if (s_pRuntime != nullptr)
    {
      if (auto* pApplication = xiiGameApplicationBase::GetGameApplicationBaseInstance())
      {
        pApplication->m_ExecutionEvents.RemoveEventHandler(&UpdateRuntime);
      }
      XII_DEFAULT_DELETE(s_pRuntime);
    }
  }
} // namespace

xiiMCPRuntime::xiiMCPRuntime() :
  m_Protocol(m_Registry),
  m_Http(m_Protocol),
  m_pDiagnostics(XII_DEFAULT_NEW(xiiMCPRuntimeDiagnostics, m_Registry))
{
  RegisterRuntimeTools(m_Registry);
}

xiiMCPRuntime::~xiiMCPRuntime()
{
  Stop();
}

xiiStatus xiiMCPRuntime::Start(const xiiMCPRuntimeConfiguration& configuration)
{
  if (configuration.m_bNative)
  {
    const auto status = m_Bridge.StartServer(m_Registry, configuration.m_sApplication, configuration.m_sLocalEndpoint, configuration.m_uiPermissions, configuration.m_bAdvertise);
    if (status.Failed())
    {
      return status;
    }
  }

  if (configuration.m_Http.m_Endpoint.GetAddress().IsValid())
  {
    const auto status = m_Http.Start(configuration.m_Http);
    if (status.Failed())
    {
      m_Bridge.Stop();
      return status;
    }
  }

  return XII_SUCCESS;
}

void xiiMCPRuntime::Update()
{
  m_pDiagnostics->Update();
  m_Bridge.Update();
  m_Http.Update();
}

void xiiMCPRuntime::Stop()
{
  m_Http.Stop();
  m_Bridge.Stop();
}

XII_BEGIN_SUBSYSTEM_DECLARATION(MCPPlugin, MCPRuntime)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    StartRuntime();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    StopRuntime();
  }

XII_END_SUBSYSTEM_DECLARATION;

XII_STATICLINK_FILE(MCPPlugin, MCPPlugin_Implementation_Runtime);
