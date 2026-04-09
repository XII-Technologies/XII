#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Types/VariantTypeRegistry.h>

void xiiRttiConverterContext::Clear()
{
  m_GuidToObject.Clear();
  m_ObjectToGuid.Clear();
  m_QueuedObjects.Clear();
}

void xiiRttiConverterContext::OnUnknownTypeError(xiiStringView sTypeName)
{
  xiiLog::Error("RTTI type '{0}' is unknown, CreateObjectFromNode failed.", sTypeName);
}

xiiUuid xiiRttiConverterContext::GenerateObjectGuid(const xiiUuid& parentGuid, const xiiAbstractProperty* pProp, xiiVariant index, void* pObject) const
{
  XII_IGNORE_UNUSED(pObject);

  xiiUuid guid = parentGuid;
  guid.HashCombine(xiiUuid::MakeStableUuidFromString(pProp->GetPropertyName()));
  if (index.IsA<xiiString>())
  {
    guid.HashCombine(xiiUuid::MakeStableUuidFromString(index.Get<xiiString>()));
  }
  else if (index.CanConvertTo<xiiUInt32>())
  {
    guid.HashCombine(xiiUuid::MakeStableUuidFromInt(index.ConvertTo<xiiUInt32>()));
  }
  else if (index.IsValid())
  {
    XII_REPORT_FAILURE("Index type must be xiiUInt32 or xiiString.");
  }
  // xiiLog::Warning("{0},{1},{2} -> {3}", parentGuid, pProp->GetPropertyName(), index, guid);
  return guid;
}

xiiInternal::NewInstance<void> xiiRttiConverterContext::CreateObject(const xiiUuid& guid, const xiiRTTI* pRtti)
{
  XII_ASSERT_DEBUG(pRtti != nullptr, "Cannot create object, RTTI type is unknown");
  if (!pRtti->GetAllocator() || !pRtti->GetAllocator()->CanAllocate())
    return nullptr;

  auto pObj = pRtti->GetAllocator()->Allocate<void>();
  RegisterObject(guid, pRtti, pObj);
  return pObj;
}

void xiiRttiConverterContext::DeleteObject(const xiiUuid& guid)
{
  auto object = GetObjectByGUID(guid);
  if (object.m_pObject)
  {
    object.m_pType->GetAllocator()->Deallocate(object.m_pObject);
  }
  UnregisterObject(guid);
}

void xiiRttiConverterContext::RegisterObject(const xiiUuid& guid, const xiiRTTI* pRtti, void* pObject)
{
  XII_ASSERT_DEV(pObject != nullptr, "cannot register null object!");
  xiiRttiConverterObject& co = m_GuidToObject[guid];

  if (pRtti->IsDerivedFrom<xiiReflectedClass>())
  {
    pRtti = static_cast<xiiReflectedClass*>(pObject)->GetDynamicRTTI();
  }

  // TODO: Actually remove child owner ptr from register when deleting an object
  // XII_ASSERT_DEV(co.m_pObject == nullptr || (co.m_pObject == pObject && co.m_pType == pRtti), "Registered same guid twice with different values");

  co.m_pObject = pObject;
  co.m_pType   = pRtti;

  m_ObjectToGuid[pObject] = guid;
}

void xiiRttiConverterContext::UnregisterObject(const xiiUuid& guid)
{
  xiiRttiConverterObject* pObj;
  if (m_GuidToObject.TryGetValue(guid, pObj))
  {
    m_GuidToObject.Remove(guid);
    m_ObjectToGuid.Remove(pObj->m_pObject);
  }
}

xiiRttiConverterObject xiiRttiConverterContext::GetObjectByGUID(const xiiUuid& guid) const
{
  xiiRttiConverterObject object;
  m_GuidToObject.TryGetValue(guid, object);
  return object;
}

xiiUuid xiiRttiConverterContext::GetObjectGUID(const xiiRTTI* pRtti, const void* pObject) const
{
  XII_IGNORE_UNUSED(pRtti);

  xiiUuid guid;

  if (pObject != nullptr)
  {
    m_ObjectToGuid.TryGetValue(pObject, guid);
  }
  return guid;
}

const xiiRTTI* xiiRttiConverterContext::FindTypeByName(xiiStringView sName) const
{
  return xiiRTTI::FindTypeByName(sName);
}

xiiUuid xiiRttiConverterContext::EnqueObject(const xiiUuid& guid, const xiiRTTI* pRtti, void* pObject)
{
  XII_ASSERT_DEBUG(guid.IsValid(), "For stable serialization, guid must be well defined");
  xiiUuid res = guid;

  if (pObject != nullptr)
  {
    // In the rare case that this succeeds we already encountered the object with a different guid before.
    // This can happen if two pointer owner point to the same object.
    if (!m_ObjectToGuid.TryGetValue(pObject, res))
    {
      RegisterObject(guid, pRtti, pObject);
    }

    m_QueuedObjects.Insert(res);
  }
  else
  {
    // Replace nullptr with invalid uuid.
    res = xiiUuid();
  }
  return res;
}

xiiRttiConverterObject xiiRttiConverterContext::DequeueObject()
{
  if (!m_QueuedObjects.IsEmpty())
  {
    auto it     = m_QueuedObjects.GetIterator();
    auto object = GetObjectByGUID(it.Key());
    XII_ASSERT_DEV(object.m_pObject != nullptr, "Enqueued object was never registered!");

    m_QueuedObjects.Remove(it);

    return object;
  }

  return xiiRttiConverterObject();
}


xiiRttiConverterWriter::xiiRttiConverterWriter(xiiAbstractObjectGraph* pGraph, xiiRttiConverterContext* pContext, bool bSerializeReadOnly, bool bSerializeOwnerPtrs)
{
  m_pGraph   = pGraph;
  m_pContext = pContext;

  m_Filter = [bSerializeReadOnly, bSerializeOwnerPtrs](const void* pObject, const xiiAbstractProperty* pProp) {
    XII_IGNORE_UNUSED(pObject);

    if (pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly) && !bSerializeReadOnly)
      return false;

    if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner) && !bSerializeOwnerPtrs)
      return false;

    return true;
  };
}

xiiRttiConverterWriter::xiiRttiConverterWriter(xiiAbstractObjectGraph* pGraph, xiiRttiConverterContext* pContext, FilterFunction filter) :
  m_pContext(pContext), m_pGraph(pGraph), m_Filter(filter)
{
  XII_ASSERT_DEBUG(filter.IsValid(), "Either filter function must be valid or a different ctor must be chosen.");
}

xiiAbstractObjectNode* xiiRttiConverterWriter::AddObjectToGraph(const xiiRTTI* pRtti, const void* pObject, xiiStringView sNodeName)
{
  const xiiUuid guid = m_pContext->GetObjectGUID(pRtti, pObject);
  XII_ASSERT_DEV(guid.IsValid(), "The object was not registered. Call xiiRttiConverterContext::RegisterObject before adding.");
  xiiAbstractObjectNode* pNode = AddSubObjectToGraph(pRtti, pObject, guid, sNodeName);

  xiiRttiConverterObject obj = m_pContext->DequeueObject();
  while (obj.m_pObject != nullptr)
  {
    const xiiUuid objectGuid = m_pContext->GetObjectGUID(obj.m_pType, obj.m_pObject);
    AddSubObjectToGraph(obj.m_pType, obj.m_pObject, objectGuid, nullptr);

    obj = m_pContext->DequeueObject();
  }

  return pNode;
}

xiiAbstractObjectNode* xiiRttiConverterWriter::AddSubObjectToGraph(const xiiRTTI* pRtti, const void* pObject, const xiiUuid& guid, xiiStringView sNodeName)
{
  xiiAbstractObjectNode* pNode = m_pGraph->AddNode(guid, pRtti->GetTypeName(), pRtti->GetTypeVersion(), sNodeName);
  AddProperties(pNode, pRtti, pObject);
  return pNode;
}

void xiiRttiConverterWriter::AddProperty(xiiAbstractObjectNode* pNode, const xiiAbstractProperty* pProp, const void* pObject)
{
  if (!m_Filter(pObject, pProp))
    return;

  xiiVariant       vTemp;
  xiiStringBuilder sTemp;
  const xiiRTTI*   pPropType    = pProp->GetSpecificType();
  const bool       bIsValueType = xiiReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case xiiPropertyCategory::Member:
    {
      const xiiAbstractMemberProperty* pSpecific = static_cast<const xiiAbstractMemberProperty*>(pProp);

      if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
      {
        vTemp                  = xiiReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
        void* pRefrencedObject = vTemp.ConvertTo<void*>();

        xiiUuid guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, xiiVariant(), pRefrencedObject);
        if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
        {
          guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          pNode->AddProperty(pProp->GetPropertyName(), guid);
        }
        else
        {
          guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);
          pNode->AddProperty(pProp->GetPropertyName(), guid);
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
        {
          vTemp = xiiReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
          xiiReflectionUtils::EnumerationToString(pPropType, vTemp.Get<xiiInt64>(), sTemp);

          pNode->AddProperty(pProp->GetPropertyName(), sTemp.GetData());
        }
        else if (bIsValueType)
        {
          pNode->AddProperty(pProp->GetPropertyName(), xiiReflectionUtils::GetMemberPropertyValue(pSpecific, pObject));
        }
        else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class) && pPropType->GetProperties().GetCount() > 0)
        {
          void* pSubObject = pSpecific->GetPropertyPointer(pObject);


          // Do we have direct access to the property?
          if (pSubObject != nullptr)
          {
            const xiiUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, xiiVariant(), pSubObject);
            pNode->AddProperty(pProp->GetPropertyName(), SubObjectGuid);

            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);
          }
          // If the property is behind an accessor, we need to retrieve it first.
          else if (pPropType->GetAllocator()->CanAllocate())
          {
            pSubObject = pPropType->GetAllocator()->Allocate<void>();

            pSpecific->GetValuePtr(pObject, pSubObject);
            const xiiUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, xiiVariant(), pSubObject);
            pNode->AddProperty(pProp->GetPropertyName(), SubObjectGuid);

            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);

            pPropType->GetAllocator()->Deallocate(pSubObject);
          }
        }
      }
    }
    break;
    case xiiPropertyCategory::Array:
    {
      const xiiAbstractArrayProperty* pSpecific = static_cast<const xiiAbstractArrayProperty*>(pProp);
      xiiUInt32                       uiCount   = pSpecific->GetCount(pObject);
      xiiVariantArray                 values;
      values.SetCount(uiCount);

      if (pSpecific->GetFlags().IsSet(xiiPropertyFlags::Pointer))
      {
        for (xiiUInt32 i = 0; i < uiCount; ++i)
        {
          vTemp                  = xiiReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
          void* pRefrencedObject = vTemp.ConvertTo<void*>();

          xiiUuid guid;
          if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
          {
            guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, i, pRefrencedObject);
            guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          }
          else
            guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);

          values[i] = guid;
        }

        pNode->AddProperty(pProp->GetPropertyName(), values);
      }
      else
      {
        if (bIsValueType)
        {
          for (xiiUInt32 i = 0; i < uiCount; ++i)
          {
            values[i] = xiiReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
          }
          pNode->AddProperty(pProp->GetPropertyName(), values);
        }
        else if (pSpecific->GetFlags().IsSet(xiiPropertyFlags::Class) && pPropType->GetAllocator()->CanAllocate())
        {
          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();

          for (xiiUInt32 i = 0; i < uiCount; ++i)
          {
            pSpecific->GetValue(pObject, i, pSubObject);
            const xiiUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, i, pSubObject);
            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);

            values[i] = SubObjectGuid;
          }
          pNode->AddProperty(pProp->GetPropertyName(), values);
          pPropType->GetAllocator()->Deallocate(pSubObject);
        }
      }
    }
    break;
    case xiiPropertyCategory::Set:
    {
      const xiiAbstractSetProperty* pSpecific = static_cast<const xiiAbstractSetProperty*>(pProp);

      xiiHybridArray<xiiVariant, 16> values;
      pSpecific->GetValues(pObject, values);

      xiiVariantArray ValuesCopied(values);

      if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
      {
        for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
        {
          void* pRefrencedObject = values[i].ConvertTo<void*>();

          xiiUuid guid;
          if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
          {
            // TODO: pointer sets are never stable unless they use an array based pseudo set as storage.
            guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, i, pRefrencedObject);
            guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          }
          else
            guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);

          ValuesCopied[i] = guid;
        }

        pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
      }
      else
      {
        if (bIsValueType)
        {
          pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
        }
      }
    }
    break;
    case xiiPropertyCategory::Map:
    {
      const xiiAbstractMapProperty* pSpecific = static_cast<const xiiAbstractMapProperty*>(pProp);

      xiiHybridArray<xiiString, 16> keys;
      pSpecific->GetKeys(pObject, keys);

      xiiVariantDictionary ValuesCopied;
      ValuesCopied.Reserve(keys.GetCount());

      if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
      {
        for (xiiUInt32 i = 0; i < keys.GetCount(); ++i)
        {
          xiiVariant value            = xiiReflectionUtils::GetMapPropertyValue(pSpecific, pObject, keys[i]);
          void*      pRefrencedObject = value.ConvertTo<void*>();

          xiiUuid guid;
          if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
          {
            guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, xiiVariant(keys[i]), pRefrencedObject);
            guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          }
          else
            guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);

          ValuesCopied.Insert(keys[i], guid);
        }

        pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
      }
      else
      {
        if (bIsValueType)
        {
          for (xiiUInt32 i = 0; i < keys.GetCount(); ++i)
          {
            xiiVariant value = xiiReflectionUtils::GetMapPropertyValue(pSpecific, pObject, keys[i]);
            ValuesCopied.Insert(keys[i], value);
          }
          pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
        }
        else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class))
        {
          for (xiiUInt32 i = 0; i < keys.GetCount(); ++i)
          {
            void* pSubObject = pPropType->GetAllocator()->Allocate<void>();
            XII_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(pSubObject););
            XII_VERIFY(pSpecific->GetValue(pObject, keys[i], pSubObject), "Key should be valid.");

            const xiiUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, xiiVariant(keys[i]), pSubObject);
            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);
            ValuesCopied.Insert(keys[i], SubObjectGuid);
          }
          pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
        }
      }
    }
    break;
    case xiiPropertyCategory::Constant:
      // Nothing to do here.
      break;
    default:
      break;
  }
}

void xiiRttiConverterWriter::AddProperties(xiiAbstractObjectNode* pNode, const xiiRTTI* pRtti, const void* pObject)
{
  if (pRtti->GetParentType())
  {
    AddProperties(pNode, pRtti->GetParentType(), pObject);
  }
  for (const auto* pProp : pRtti->GetProperties())
  {
    AddProperty(pNode, pProp, pObject);
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_RttiConverterWriter);
