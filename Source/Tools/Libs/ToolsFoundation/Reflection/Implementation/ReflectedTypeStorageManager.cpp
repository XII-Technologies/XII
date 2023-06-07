#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

xiiMap<const xiiRTTI*, xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping*> xiiReflectedTypeStorageManager::s_ReflectedTypeToStorageMapping;

// clang-format off
// 
XII_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, ReflectedTypeStorageManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiReflectedTypeStorageManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiReflectedTypeStorageManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping public functions
////////////////////////////////////////////////////////////////////////

void xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddProperties(const xiiRTTI* pType)
{
  // Mark all properties as invalid. Thus, when a property is dropped we know it is no longer valid.
  // All others will be set to their old or new value by the AddPropertiesRecursive function.
  for (auto it = m_PathToStorageInfoTable.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Type = xiiVariant::Type::Invalid;
  }

  xiiSet<const xiiDocumentObject*> requiresPatchingEmbeddedClass;
  AddPropertiesRecursive(pType, requiresPatchingEmbeddedClass);

  for (const xiiDocumentObject* pObject : requiresPatchingEmbeddedClass)
  {
    pObject->GetDocumentObjectManager()->PatchEmbeddedClassObjects(pObject);
  }
}

void xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddPropertiesRecursive(
  const xiiRTTI*                    pType,
  xiiSet<const xiiDocumentObject*>& ref_requiresPatchingEmbeddedClass)
{
  // Parse parent class
  const xiiRTTI* pParent = pType->GetParentType();
  if (pParent != nullptr)
    AddPropertiesRecursive(pParent, ref_requiresPatchingEmbeddedClass);

  // Parse properties
  const xiiUInt32 uiPropertyCount = pType->GetProperties().GetCount();
  for (xiiUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const xiiAbstractProperty* pProperty = pType->GetProperties()[i];

    xiiString path = pProperty->GetPropertyName();

    StorageInfo* storageInfo = nullptr;
    if (m_PathToStorageInfoTable.TryGetValue(path, storageInfo))
    {
      // Value already present, update type and instances
      storageInfo->m_Type         = GetStorageType(pProperty);
      storageInfo->m_DefaultValue = xiiToolsReflectionUtils::GetStorageDefault(pProperty);
      UpdateInstances(storageInfo->m_uiIndex, pProperty, ref_requiresPatchingEmbeddedClass);
    }
    else
    {
      const xiiUInt16 uiIndex = (xiiUInt16)m_PathToStorageInfoTable.GetCount();

      // Add value, new entries are appended
      m_PathToStorageInfoTable.Insert(path, StorageInfo(uiIndex, GetStorageType(pProperty), xiiToolsReflectionUtils::GetStorageDefault(pProperty)));
      AddPropertyToInstances(uiIndex, pProperty, ref_requiresPatchingEmbeddedClass);
    }
  }
}

void xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::UpdateInstances(
  xiiUInt32                         uiIndex,
  const xiiAbstractProperty*        pProperty,
  xiiSet<const xiiDocumentObject*>& ref_requiresPatchingEmbeddedClass)
{
  for (auto it = m_Instances.GetIterator(); it.IsValid(); ++it)
  {
    xiiDynamicArray<xiiVariant>& data = it.Key()->m_Data;
    XII_ASSERT_DEV(uiIndex < data.GetCount(), "xiiReflectedTypeStorageAccessor found with fewer properties that is should have!");
    xiiVariant& value = data[uiIndex];

    const auto SpecVarType = GetStorageType(pProperty);

    switch (pProperty->GetCategory())
    {
      case xiiPropertyCategory::Member:
      {
        if (pProperty->GetFlags().IsSet(xiiPropertyFlags::Class) && !pProperty->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        {
          // Did the type change from what it was previously?
          if (value.GetType() == SpecVarType)
          {
            if (!value.Get<xiiUuid>().IsValid())
            {
              ref_requiresPatchingEmbeddedClass.Insert(it.Key()->GetOwner());
            }
          }
          else
          {
            value = xiiToolsReflectionUtils::GetStorageDefault(pProperty);
            ref_requiresPatchingEmbeddedClass.Insert(it.Key()->GetOwner());
          }
          continue;
        }
        else
        {
          // Did the type change from what it was previously?
          if (value.GetType() == SpecVarType)
          {
            // The types are equal so nothing needs to be done. The current value will stay valid.
            // This should be the most common case.
            continue;
          }
          else
          {
            // The type is new or has changed but we have a valid value stored. Assume that the type of a property was changed
            // and try to convert the value.
            if (value.CanConvertTo(SpecVarType))
            {
              value = value.ConvertTo(SpecVarType);
            }
            else
            {
              value = xiiToolsReflectionUtils::GetStorageDefault(pProperty);
            }
            continue;
          }
        }
      }
      break;
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      {
        if (value.GetType() != xiiVariantType::VariantArray)
        {
          value = xiiVariantArray();
          continue;
        }
        xiiVariantArray values = value.Get<xiiVariantArray>();
        if (values.IsEmpty())
          continue;

        // Same conversion logic as for xiiPropertyCategory::Member, but for each element instead.
        for (xiiUInt32 i = 0; i < values.GetCount(); i++)
        {
          xiiVariant& var = values[i];
          if (var.GetType() == SpecVarType)
          {
            continue;
          }
          else
          {
            xiiResult res(XII_FAILURE);
            var = var.ConvertTo(SpecVarType, &res);
            if (res == XII_FAILURE)
            {
              var = xiiReflectionUtils::GetDefaultValue(pProperty, i);
            }
          }
        }
        value = values;
      }
      break;
      case xiiPropertyCategory::Map:
      {
        if (value.GetType() != xiiVariantType::VariantDictionary)
        {
          value = xiiVariantDictionary();
          continue;
        }
        xiiVariantDictionary values = value.Get<xiiVariantDictionary>();
        if (values.IsEmpty())
          continue;

        // Same conversion logic as for xiiPropertyCategory::Member, but for each element instead.
        for (auto it2 = values.GetIterator(); it2.IsValid(); ++it2)
        {
          if (it2.Value().GetType() == SpecVarType)
          {
            continue;
          }
          else
          {
            xiiResult res(XII_FAILURE);
            it2.Value() = it2.Value().ConvertTo(SpecVarType, &res);
            if (res == XII_FAILURE)
            {
              it2.Value() = xiiReflectionUtils::GetDefaultValue(pProperty, it2.Key());
            }
          }
        }
        value = values;
      }
      break;
      default:
        break;
    }
  }
}

void xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddPropertyToInstances(
  xiiUInt32                         uiIndex,
  const xiiAbstractProperty*        pProperty,
  xiiSet<const xiiDocumentObject*>& ref_requiresPatchingEmbeddedClass)
{
  if (pProperty->GetCategory() != xiiPropertyCategory::Member)
    return;

  for (auto it = m_Instances.GetIterator(); it.IsValid(); ++it)
  {
    xiiDynamicArray<xiiVariant>& data = it.Key()->m_Data;
    XII_ASSERT_DEV(data.GetCount() == uiIndex, "xiiReflectedTypeStorageAccessor found with a property count that does not match its storage mapping!");
    data.PushBack(xiiToolsReflectionUtils::GetStorageDefault(pProperty));
    if (pProperty->GetFlags().IsSet(xiiPropertyFlags::Class) && !pProperty->GetFlags().IsSet(xiiPropertyFlags::Pointer))
    {
      ref_requiresPatchingEmbeddedClass.Insert(it.Key()->GetOwner());
    }
  }
}


xiiVariantType::Enum xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::GetStorageType(const xiiAbstractProperty* pProperty)
{
  xiiVariantType::Enum type = xiiVariantType::Uuid;

  const bool bIsValueType = xiiReflectionUtils::IsValueType(pProperty);

  switch (pProperty->GetCategory())
  {
    case xiiPropertyCategory::Member:
    {
      if (bIsValueType)
        type = pProperty->GetSpecificType()->GetVariantType();
      else if (pProperty->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
        type = xiiVariantType::Int64;
    }
    break;
    case xiiPropertyCategory::Array:
    case xiiPropertyCategory::Set:
    {
      type = xiiVariantType::VariantArray;
    }
    break;
    case xiiPropertyCategory::Map:
    {
      type = xiiVariantType::VariantDictionary;
    }
    break;
    default:
      break;
  }

  return type;
}

////////////////////////////////////////////////////////////////////////
// xiiReflectedTypeStorageManager private functions
////////////////////////////////////////////////////////////////////////

void xiiReflectedTypeStorageManager::Startup()
{
  xiiPhantomRttiManager::s_Events.AddEventHandler(TypeEventHandler);
}

void xiiReflectedTypeStorageManager::Shutdown()
{
  xiiPhantomRttiManager::s_Events.RemoveEventHandler(TypeEventHandler);

  for (auto it = s_ReflectedTypeToStorageMapping.GetIterator(); it.IsValid(); ++it)
  {
    ReflectedTypeStorageMapping* pMapping = it.Value();

    for (auto inst : pMapping->m_Instances)
    {
      const char* sz = inst->GetType()->GetTypeName();
      xiiLog::Error("Type '{0}' survived shutdown!", sz);
    }

    XII_ASSERT_DEV(pMapping->m_Instances.IsEmpty(), "A type was removed which still has instances using the type!");
    XII_DEFAULT_DELETE(pMapping);
  }
  s_ReflectedTypeToStorageMapping.Clear();
}

const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping* xiiReflectedTypeStorageManager::AddStorageAccessor(
  xiiReflectedTypeStorageAccessor* pInstance)
{
  ReflectedTypeStorageMapping* pMapping = GetTypeStorageMapping(pInstance->GetType());
  pMapping->m_Instances.Insert(pInstance);
  return pMapping;
}

void xiiReflectedTypeStorageManager::RemoveStorageAccessor(xiiReflectedTypeStorageAccessor* pInstance)
{
  ReflectedTypeStorageMapping* pMapping = GetTypeStorageMapping(pInstance->GetType());
  pMapping->m_Instances.Remove(pInstance);
}

xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping* xiiReflectedTypeStorageManager::GetTypeStorageMapping(const xiiRTTI* pType)
{
  XII_ASSERT_DEV(pType != nullptr, "Nullptr is not a valid type!");
  auto it = s_ReflectedTypeToStorageMapping.Find(pType);
  if (it.IsValid())
    return it.Value();

  ReflectedTypeStorageMapping* pMapping = XII_DEFAULT_NEW(ReflectedTypeStorageMapping);
  pMapping->AddProperties(pType);
  s_ReflectedTypeToStorageMapping[pType] = pMapping;
  return pMapping;
}

void xiiReflectedTypeStorageManager::TypeEventHandler(const xiiPhantomRttiManagerEvent& e)
{
  switch (e.m_Type)
  {
    case xiiPhantomRttiManagerEvent::Type::TypeAdded:
    {
      const xiiRTTI* pType = e.m_pChangedType;
      XII_ASSERT_DEV(pType != nullptr, "A type was added but it has an invalid handle!");

      XII_ASSERT_DEV(!s_ReflectedTypeToStorageMapping.Find(e.m_pChangedType).IsValid(), "The type '{0}' was added twice!", pType->GetTypeName());
      GetTypeStorageMapping(e.m_pChangedType);
    }
    break;
    case xiiPhantomRttiManagerEvent::Type::TypeChanged:
    {
      const xiiRTTI* pNewType = e.m_pChangedType;
      XII_ASSERT_DEV(pNewType != nullptr, "A type was updated but its handle is invalid!");

      ReflectedTypeStorageMapping* pMapping = s_ReflectedTypeToStorageMapping[e.m_pChangedType];
      XII_ASSERT_DEV(pMapping != nullptr, "A type was updated but no mapping exists for it!");

      if (pNewType->GetParentType() != nullptr && xiiStringUtils::IsEqual(pNewType->GetParentType()->GetTypeName(), "xiiEnumBase"))
      {
        // XII_ASSERT_DEV(false, "Updating enums not implemented yet!");
        break;
      }
      else if (pNewType->GetParentType() != nullptr && xiiStringUtils::IsEqual(pNewType->GetParentType()->GetTypeName(), "xiiBitflagsBase"))
      {
        XII_ASSERT_DEV(false, "Updating bitflags not implemented yet!");
      }

      pMapping->AddProperties(pNewType);

      xiiSet<xiiRTTI*> dependencies;
      // Update all types that either derive from the changed type or have the type as a member.
      for (auto it = s_ReflectedTypeToStorageMapping.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Key() == e.m_pChangedType)
          continue;

        const xiiRTTI* pType = it.Key();
        if (pType->IsDerivedFrom(e.m_pChangedType))
        {
          it.Value()->AddProperties(pType);
        }
      }
    }
    break;
    case xiiPhantomRttiManagerEvent::Type::TypeRemoved:
    {
      ReflectedTypeStorageMapping* pMapping = s_ReflectedTypeToStorageMapping[e.m_pChangedType];
      XII_ASSERT_DEV(pMapping != nullptr, "A type was removed but no mapping ever exited for it!");
      XII_ASSERT_DEV(pMapping->m_Instances.IsEmpty(), "A type was removed which still has instances using the type!");
      s_ReflectedTypeToStorageMapping.Remove(e.m_pChangedType);
      XII_DEFAULT_DELETE(pMapping);
    }
    break;
  }
}
