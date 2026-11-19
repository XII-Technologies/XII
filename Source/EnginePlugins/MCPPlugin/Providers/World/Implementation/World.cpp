/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCPPlugin/MCPPluginPCH.h>

#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <MCPPlugin/Json.h>
#include <MCPPlugin/Providers/RuntimeProviders.h>
#include <MCPPlugin/Providers/World/World.h>
#include <MCPPlugin/Reflection.h>

xiiWorld* xiiMCPRuntimeProviders::FindWorld(const xiiVariantDictionary& arguments)
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

namespace
{
  using xiiMCPRuntimeProviders::FindWorld;
  using xiiMCPRuntimeProviders::FormatHandle;

  constexpr xiiUInt32 DefaultPageSize = 32;
  constexpr xiiUInt32 MaximumPageSize = 128;

  xiiString WorldId(const xiiWorld& world)
  {
    xiiStringBuilder sId;
    xiiConversionUtils::ToString(world.GetInstanceId(), sId);
    return xiiString(sId);
  }

  xiiGameObject* FindObject(xiiWorld& ref_world, xiiStringView sObject)
  {
    if (sObject.IsEmpty())
    {
      return nullptr;
    }

    xiiGameObjectId id;
    if (xiiConversionUtils::StringToUInt64Strict(sObject, id.m_Data).Failed() || id.m_WorldIndex != ref_world.GetIndex())
    {
      return nullptr;
    }
    xiiGameObject* pObject = nullptr;
    return ref_world.TryGetObject(xiiGameObjectHandle(id), pObject) ? pObject : nullptr;
  }

  xiiVariantDictionary DescribeObject(const xiiGameObject& object)
  {
    xiiVariantDictionary result;
    result["id"]             = FormatHandle(object.GetHandle());
    result["name"]           = xiiString(object.GetName());
    result["active"]         = object.IsActive();
    result["dynamic"]        = object.IsDynamic();
    result["componentCount"] = object.GetComponents().GetCount();
    if (const auto* pParent = object.GetParent())
    {
      result["parent"] = FormatHandle(pParent->GetHandle());
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
      if (!sParent.IsEmpty() && (it->GetParent() == nullptr || FormatHandle(it->GetParent()->GetHandle()) != sParent))
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
      component["id"]     = FormatHandle(pComponent->GetHandle());
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
    out_result["id"]         = FormatHandle(pComponent->GetHandle());
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

  xiiStatus EditComponent(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    auto* pWorld = FindWorld(arguments);
    if (pWorld == nullptr)
    {
      return xiiStatus("Select a current world.");
    }

    XII_LOCK(pWorld->GetWriteMarker());
    auto* pObject = FindObject(*pWorld, xiiMCPJson::GetString(arguments, "object"));
    if (pObject == nullptr || context.HasExpired())
    {
      return xiiStatus("The owner is unavailable or the operation expired.");
    }

    const auto sAction = xiiMCPJson::GetString(arguments, "action");
    if (sAction == "create")
    {
      const auto* pType = xiiRTTI::FindTypeByName(xiiMCPJson::GetString(arguments, "type"));
      if (!pObject->IsDynamic() || pType == nullptr || !pType->IsDerivedFrom<xiiComponent>() || pType->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
      {
        return xiiStatus("Creation requires a dynamic owner and a concrete registered component type.");
      }
      auto* pManager = pWorld->GetOrCreateManagerForComponentType(pType);
      if (pManager == nullptr || pManager->GetComponentCount() >= xiiWorld::GetMaxNumComponentsPerType())
      {
        return xiiStatus("The component manager is unavailable or full.");
      }
      const auto hComponent = pManager->CreateComponent(pObject);
      if (hComponent.IsInvalidated())
      {
        return xiiStatus("The component manager rejected creation.");
      }
      out_result["id"] = FormatHandle(hComponent);
      return XII_SUCCESS;
    }

    xiiUInt64 uiComponent = 0;
    if (xiiConversionUtils::StringToUInt64Strict(xiiMCPJson::GetString(arguments, "component"), uiComponent).Failed())
    {
      return xiiStatus("Use a component ID returned by runtime.object.");
    }
    for (auto* pComponent : pObject->GetComponents())
    {
      if (pComponent->GetHandle().GetInternalID().m_Data != uiComponent)
      {
        continue;
      }
      if (sAction == "remove")
      {
        pComponent->DeleteComponent();
        out_result["removed"] = true;
        return XII_SUCCESS;
      }
      if (sAction == "set")
      {
        const auto& type      = *pComponent->GetDynamicRTTI();
        const auto  sProperty = xiiMCPJson::GetString(arguments, "property");
        const auto* pValue    = arguments.GetValue("value");
        if (type.GetTypeName() != xiiMCPJson::GetString(arguments, "type") || sProperty.IsEmpty() || pValue == nullptr)
        {
          return xiiStatus("Provide the component's current type, a property name and its new value.");
        }

        const auto sId = FormatHandle(pComponent->GetHandle());
        XII_SUCCEED_OR_RETURN(xiiMCPReflection::SetRuntimeMember(type, pComponent, sProperty, *pValue));
        out_result["id"]       = sId;
        out_result["property"] = xiiString(sProperty);
        return XII_SUCCESS;
      }
      const auto* pActive = arguments.GetValue("active");
      if (sAction == "activate" && pActive != nullptr && pActive->IsA<bool>())
      {
        pComponent->SetActiveFlag(pActive->Get<bool>());
        out_result["activeFlag"] = pActive->Get<bool>();
        return XII_SUCCESS;
      }
      return xiiStatus("Provide a supported action and a boolean active flag for activation.");
    }
    return xiiStatus("The component no longer belongs to the selected object.");
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

  xiiStatus ReparentObject(const xiiMCPCallContext& context, xiiGameObject& ref_object, const xiiVariant& parent)
  {
    xiiGameObject* pParent = nullptr;
    if (parent.IsValid())
    {
      if (!parent.IsA<xiiString>() || (pParent = FindObject(*ref_object.GetWorld(), parent.Get<xiiString>())) == nullptr)
      {
        return xiiStatus("The parent must be null or a current object ID in the same world.");
      }
    }
    if (ref_object.GetParent() == pParent)
    {
      return XII_SUCCESS;
    }
    if (ref_object.IsStatic() && pParent != nullptr && pParent->IsDynamic())
    {
      return xiiStatus("A static object cannot be attached to a dynamic parent.");
    }

    xiiUInt32 uiNewDepth = 0;
    for (const auto* pAncestor = pParent; pAncestor != nullptr; pAncestor = pAncestor->GetParent())
    {
      if (pAncestor == &ref_object)
      {
        return xiiStatus("This parent would create a hierarchy cycle.");
      }
      ++uiNewDepth;
    }
    if (uiNewDepth >= xiiWorld::GetMaxNumHierarchyLevels())
    {
      return xiiStatus("The resulting object hierarchy would exceed the world limit.");
    }

    xiiHybridArray<xiiGameObject::ConstChildIterator, 16> subtree;
    subtree.PushBack(static_cast<const xiiGameObject&>(ref_object).GetChildren());
    xiiUInt32 uiVisited = 0;
    while (!subtree.IsEmpty())
    {
      auto& it = subtree.PeekBack();
      if (!it.IsValid())
      {
        subtree.PopBack();
        continue;
      }
      if (uiNewDepth + subtree.GetCount() >= xiiWorld::GetMaxNumHierarchyLevels())
      {
        return xiiStatus("The resulting object hierarchy would exceed the world limit.");
      }
      if ((uiVisited++ % 128) == 0 && context.HasExpired())
      {
        return xiiStatus("The reparent deadline expired while checking descendants.");
      }

      const auto children = it->GetChildren();
      ++it;
      subtree.PushBack(children);
    }
    if (context.HasExpired())
    {
      return xiiStatus("The reparent deadline expired.");
    }

    const xiiGameObjectHandle hParent = pParent != nullptr ? pParent->GetHandle() : xiiGameObjectHandle();
    ref_object.SetParent(hParent, xiiTransformPreservation::PreserveLocal);
    return XII_SUCCESS;
  }

  xiiStatus EditObject(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    auto*       pWorld       = FindWorld(arguments);
    const auto  sAction      = xiiMCPJson::GetString(arguments, "action");
    const auto  sName        = xiiMCPJson::GetString(arguments, "name");
    const auto* pPosition    = arguments.GetValue("position");
    const auto* pRotation    = arguments.GetValue("rotation");
    const auto* pScale       = arguments.GetValue("scale");
    const auto* pActive      = arguments.GetValue("active");
    const auto* pDynamic     = arguments.GetValue("dynamic");
    xiiVec3     vPosition    = xiiVec3::MakeZero();
    xiiVec3     vScale       = xiiVec3(1.0f);
    float       fRotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    if (pWorld == nullptr || sName.GetElementCount() > 128 || (pPosition != nullptr && !ReadPosition(*pPosition, vPosition)) ||
        (pScale != nullptr && (!ReadPosition(*pScale, vScale) || vScale.x == 0.0f || vScale.y == 0.0f || vScale.z == 0.0f)) ||
        (pRotation != nullptr && !xiiMCPReflection::ReadFloatArray(*pRotation, fRotation)) ||
        (pActive != nullptr && !pActive->IsA<bool>()) || (pDynamic != nullptr && (sAction != "create" || !pDynamic->IsA<bool>())))
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
      description.m_bDynamic = pDynamic != nullptr && pDynamic->Get<bool>();
      pWorld->CreateObject(description, pObject);
    }
    else if (sAction == "modify" || sAction == "remove" || sAction == "reparent")
    {
      pObject = FindObject(*pWorld, xiiMCPJson::GetString(arguments, "object"));
    }
    if (pObject == nullptr)
    {
      return xiiStatus("The action or object is unavailable. Inspect the current world.");
    }

    if (sAction == "remove")
    {
      out_result["id"] = FormatHandle(pObject->GetHandle());
      pWorld->DeleteObjectDelayed(pObject->GetHandle(), false);
      out_result["removalScheduled"] = true;
      return XII_SUCCESS;
    }
    if (sAction == "reparent")
    {
      const auto* pParent = arguments.GetValue("parent");
      if (pParent == nullptr)
      {
        return xiiStatus("Provide a parent object ID, or null to move the object to the root.");
      }
      XII_SUCCEED_OR_RETURN(ReparentObject(context, *pObject, *pParent));
      out_result = DescribeObject(*pObject);
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

  xiiStatus ReadWorldSlot(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto sIndex  = xiiMCPJson::GetString(arguments, "index");
    xiiUInt64  uiIndex = 0;
    if (xiiConversionUtils::StringToUInt64Strict(sIndex, uiIndex).Failed() || uiIndex >= xiiWorld::GetWorldCount())
    {
      return xiiStatus("The specified world slot is unavailable.");
    }
    const auto* pWorld = xiiWorld::GetWorld(static_cast<xiiUInt32>(uiIndex));
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
    return xiiMCPRuntimeProviders::WriteResource(xiiMCPJson::GetString(arguments, "uri"), data, out_result);
  }

  xiiStatus EditObjects(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
  {
    const auto  sWorld      = xiiMCPJson::GetString(arguments, "world");
    const auto* pOperations = arguments.GetValue("operations");
    if (FindWorld(arguments) == nullptr || pOperations == nullptr || !pOperations->IsA<xiiVariantArray>())
    {
      return xiiStatus("Provide a current world and an array of operations.");
    }
    const auto& operations = pOperations->Get<xiiVariantArray>();
    if (operations.IsEmpty() || operations.GetCount() > 24)
    {
      return xiiStatus("Provide between 1 and 24 operations.");
    }

    xiiVariantArray results;
    for (const auto& operation : operations)
    {
      if (context.HasExpired())
      {
        out_result["expired"] = true;
        break;
      }

      xiiVariantDictionary result;
      if (!operation.IsA<xiiVariantDictionary>())
      {
        result["error"] = xiiString("Each operation must be an object.");
      }
      else
      {
        auto parameters     = operation.Get<xiiVariantDictionary>();
        parameters["world"] = xiiString(sWorld);
        const auto status   = EditObject(context, parameters, result);
        if (status.Failed())
        {
          result["error"] = status.GetMessageString();
        }
      }
      const bool bFailed = result.Contains("error");
      results.PushBack(result);
      if (bFailed)
      {
        break;
      }
    }
    out_result["complete"]   = results.GetCount() == operations.GetCount() && !results.PeekBack().Get<xiiVariantDictionary>().Contains("error");
    out_result["operations"] = results;
    return XII_SUCCESS;
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
} // namespace

void xiiMCPRuntimeProviders::RegisterWorld(xiiMCPRegistry& ref_registry)
{
  const auto read    = xiiMCPPermission::Read;
  const auto edit    = xiiMCPPermission::Edit;
  const auto execute = xiiMCPPermission::Execute;
  ref_registry.RegisterTool("runtime.worlds", "List live worlds and simulation timing. World IDs are valid for the lifetime of each world.", R"({"type":"object","additionalProperties":false})", read, &ListWorlds).IgnoreResult();
  ref_registry.RegisterTool("runtime.objects", "List live game objects using a bounded page and optional filters. Object IDs belong to the specified world.", R"({"type":"object","required":["world"],"properties":{"world":{"type":"string"},"name":{"type":"string"},"parent":{"type":"string"},"componentType":{"type":"string"},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":128,"default":32}}})", read, &ListObjects).IgnoreResult();
  ref_registry.RegisterTool("runtime.object", "Inspect one live game object, its local and global transforms, and a page of components.", R"({"type":"object","required":["world","object"],"properties":{"world":{"type":"string"},"object":{"type":"string"},"offset":{"type":"integer","minimum":0},"limit":{"type":"integer","minimum":1,"maximum":32,"default":32}}})", read, &InspectObject).IgnoreResult();
  ref_registry.RegisterTool("runtime.component", "Inspect supported reflected properties on a live component. Discover its index and type with runtime.object.", R"({"type":"object","required":["world","object","index","type"],"properties":{"world":{"type":"string"},"object":{"type":"string"},"index":{"type":"integer","minimum":0},"type":{"type":"string"}}})", read, &InspectComponent).IgnoreResult();
  ref_registry.RegisterTool("runtime.edit_component", "Attach a component to a dynamic owner, activate or remove a component by its current ID, or set a property marked x-xii-runtime-editable in runtime.component. Set requires the component's current ID and type, a property name and its new value.", R"({"type":"object","required":["world","object","action"],"properties":{"world":{"type":"string"},"object":{"type":"string"},"action":{"enum":["create","activate","remove","set"]},"type":{"type":"string"},"component":{"type":"string"},"active":{"type":"boolean"},"property":{"type":"string"},"value":{}}})", edit, &EditComponent).IgnoreResult();
  ref_registry.RegisterTool("runtime.edit_object", "Create, modify, reparent or schedule removal of a live object. Reparenting preserves the local transform and accepts null to move to the root. Rotation is a normalized quaternion in x,y,z,w order.", R"({"type":"object","required":["world","action"],"properties":{"world":{"type":"string"},"action":{"enum":["create","modify","reparent","remove"]},"object":{"type":"string"},"parent":{"type":["string","null"]},"name":{"type":"string","maxLength":128},"position":{"type":"array","items":{"type":"number"},"minItems":3,"maxItems":3},"rotation":{"type":"array","items":{"type":"number"},"minItems":4,"maxItems":4},"scale":{"type":"array","items":{"type":"number"},"minItems":3,"maxItems":3},"active":{"type":"boolean"},"dynamic":{"type":"boolean","description":"Creation only. Required for component attachment."}}})", edit, &EditObject).IgnoreResult();
  ref_registry.RegisterTool("runtime.simulation", "Set simulation enabled, clock pause, or simulation speed for a live world and return its current timing.", R"({"type":"object","required":["world"],"properties":{"world":{"type":"string"},"enabled":{"type":"boolean"},"paused":{"type":"boolean"},"speed":{"type":"number","minimum":0,"maximum":16}}})", execute, &SetSimulation).IgnoreResult();
  ref_registry.RegisterTool("runtime.edit_objects", "Run up to 24 runtime.edit_object operations on one world in sequence. Successful edits remain applied if an operation fails or the deadline expires.",
                            R"({"type":"object","required":["world","operations"],"properties":{"world":{"type":"string"},"operations":{"type":"array","minItems":1,"maxItems":24,"items":{"type":"object"}}}})", edit, &EditObjects)
    .IgnoreResult();

  xiiVariantDictionary worldSlot;
  worldSlot["name"]        = xiiString("Live world slot");
  worldSlot["description"] = xiiString("Snapshot of the world currently occupying a numeric registry slot. Use its returned ID for operations.");
  worldSlot["mimeType"]    = xiiString("application/json");
  ref_registry.Register(xiiMCPCapability::ResourceTemplate, "xii://runtime/worlds/{index}", std::move(worldSlot), read, &ReadWorldSlot).IgnoreResult();

  xiiVariantDictionary prompt;
  prompt["description"] = xiiString("Walk through inspection of a live world, its object hierarchy and reflected components.");
  ref_registry.Register(xiiMCPCapability::Prompt, "runtime.inspect_scene", std::move(prompt), read, &SceneInspectionPrompt).IgnoreResult();
}
