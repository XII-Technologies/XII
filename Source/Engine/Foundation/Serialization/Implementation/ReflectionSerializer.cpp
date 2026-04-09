#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Types/VariantTypeRegistry.h>

////////////////////////////////////////////////////////////////////////
// xiiReflectionSerializer public static functions
////////////////////////////////////////////////////////////////////////

void xiiReflectionSerializer::WriteObjectToDDL(xiiStreamWriter& ref_stream, const xiiRTTI* pRtti, const void* pObject, bool bCompactMmode /*= true*/, xiiOpenDdlWriter::TypeStringMode typeMode /*= xiiOpenDdlWriter::TypeStringMode::Shortest*/)
{
  xiiAbstractObjectGraph  graph;
  xiiRttiConverterContext context;
  xiiRttiConverterWriter  conv(&graph, &context, false, true);

  context.RegisterObject(xiiUuid::MakeUuid(), pRtti, const_cast<void*>(pObject));
  conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");

  xiiAbstractGraphDdlSerializer::Write(ref_stream, &graph, nullptr, bCompactMmode, typeMode);
}

void xiiReflectionSerializer::WriteObjectToDDL(xiiOpenDdlWriter& ref_ddl, const xiiRTTI* pRtti, const void* pObject, xiiUuid guid /*= xiiUuid()*/)
{
  xiiAbstractObjectGraph  graph;
  xiiRttiConverterContext context;
  xiiRttiConverterWriter  conv(&graph, &context, false, true);

  if (!guid.IsValid())
  {
    guid = xiiUuid::MakeUuid();
  }

  context.RegisterObject(guid, pRtti, const_cast<void*>(pObject));
  conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");

  xiiAbstractGraphDdlSerializer::Write(ref_ddl, &graph, nullptr);
}

void xiiReflectionSerializer::WriteObjectToBinary(xiiStreamWriter& ref_stream, const xiiRTTI* pRtti, const void* pObject)
{
  xiiAbstractObjectGraph  graph;
  xiiRttiConverterContext context;
  xiiRttiConverterWriter  conv(&graph, &context, false, true);

  context.RegisterObject(xiiUuid::MakeUuid(), pRtti, const_cast<void*>(pObject));
  conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");

  xiiAbstractGraphBinarySerializer::Write(ref_stream, &graph);
}

void* xiiReflectionSerializer::ReadObjectFromDDL(xiiStreamReader& ref_stream, const xiiRTTI*& ref_pRtti)
{
  xiiOpenDdlReader reader;
  if (reader.ParseDocument(ref_stream, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
  {
    xiiLog::Error("Failed to parse DDL graph");
    return nullptr;
  }

  return ReadObjectFromDDL(reader.GetRootElement(), ref_pRtti);
}

void* xiiReflectionSerializer::ReadObjectFromDDL(const xiiOpenDdlReaderElement* pRootElement, const xiiRTTI*& ref_pRtti)
{
  xiiAbstractObjectGraph  graph;
  xiiRttiConverterContext context;

  xiiAbstractGraphDdlSerializer::Read(pRootElement, &graph).IgnoreResult();

  xiiRttiConverterReader convRead(&graph, &context);
  auto*                  pRootNode = graph.GetNodeByName("root");

  XII_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  ref_pRtti = xiiRTTI::FindTypeByName(pRootNode->GetType());

  void* pTarget = context.CreateObject(pRootNode->GetGuid(), ref_pRtti);

  convRead.ApplyPropertiesToObject(pRootNode, ref_pRtti, pTarget);

  return pTarget;
}

void* xiiReflectionSerializer::ReadObjectFromBinary(xiiStreamReader& ref_stream, const xiiRTTI*& ref_pRtti)
{
  xiiAbstractObjectGraph  graph;
  xiiRttiConverterContext context;

  xiiAbstractGraphBinarySerializer::Read(ref_stream, &graph);

  xiiRttiConverterReader convRead(&graph, &context);
  auto*                  pRootNode = graph.GetNodeByName("root");

  XII_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  ref_pRtti = xiiRTTI::FindTypeByName(pRootNode->GetType());

  void* pTarget = context.CreateObject(pRootNode->GetGuid(), ref_pRtti);

  convRead.ApplyPropertiesToObject(pRootNode, ref_pRtti, pTarget);

  return pTarget;
}

void xiiReflectionSerializer::ReadObjectPropertiesFromDDL(xiiStreamReader& ref_stream, const xiiRTTI& rtti, void* pObject)
{
  xiiAbstractObjectGraph  graph;
  xiiRttiConverterContext context;

  xiiAbstractGraphDdlSerializer::Read(ref_stream, &graph).IgnoreResult();

  xiiRttiConverterReader convRead(&graph, &context);
  auto*                  pRootNode = graph.GetNodeByName("root");

  XII_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  if (pRootNode == nullptr)
    return;

  convRead.ApplyPropertiesToObject(pRootNode, &rtti, pObject);
}

void xiiReflectionSerializer::ReadObjectPropertiesFromBinary(xiiStreamReader& ref_stream, const xiiRTTI& rtti, void* pObject)
{
  xiiAbstractObjectGraph  graph;
  xiiRttiConverterContext context;

  xiiAbstractGraphBinarySerializer::Read(ref_stream, &graph);

  xiiRttiConverterReader convRead(&graph, &context);
  auto*                  pRootNode = graph.GetNodeByName("root");

  XII_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  convRead.ApplyPropertiesToObject(pRootNode, &rtti, pObject);
}


namespace
{
  static void CloneProperty(const void* pObject, void* pClone, const xiiAbstractProperty* pProp)
  {
    if (pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
      return;

    const xiiRTTI* pPropType = pProp->GetSpecificType();

    const bool bIsValueType = xiiReflectionUtils::IsValueType(pProp);

    xiiVariant vTemp;
    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Member:
      {
        auto pSpecific = static_cast<const xiiAbstractMemberProperty*>(pProp);

        if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        {
          vTemp = xiiReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);

          void* pRefrencedObject = vTemp.ConvertTo<void*>();
          if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner) && pRefrencedObject)
          {
            pRefrencedObject = xiiReflectionSerializer::Clone(pRefrencedObject, pPropType);
            vTemp            = xiiVariant(pRefrencedObject, pPropType);
          }

          xiiVariant vOldValue = xiiReflectionUtils::GetMemberPropertyValue(pSpecific, pClone);
          xiiReflectionUtils::SetMemberPropertyValue(pSpecific, pClone, vTemp);
          if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
            xiiReflectionUtils::DeleteObject(vOldValue.ConvertTo<void*>(), pProp);
        }
        else
        {
          if (bIsValueType || pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
          {
            vTemp = xiiReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
            xiiReflectionUtils::SetMemberPropertyValue(pSpecific, pClone, vTemp);
          }
          else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class))
          {
            void* pSubObject = pSpecific->GetPropertyPointer(pObject);
            // Do we have direct access to the property?
            if (pSubObject != nullptr)
            {
              void* pSubClone = pSpecific->GetPropertyPointer(pClone);
              xiiReflectionSerializer::Clone(pSubObject, pSubClone, pPropType);
            }
            // If the property is behind an accessor, we need to retrieve it first.
            else if (pPropType->GetAllocator()->CanAllocate())
            {
              pSubObject = pPropType->GetAllocator()->Allocate<void>();
              pSpecific->GetValuePtr(pObject, pSubObject);
              pSpecific->SetValuePtr(pClone, pSubObject);
              pPropType->GetAllocator()->Deallocate(pSubObject);
            }
          }
        }
      }
      break;
      case xiiPropertyCategory::Array:
      {
        auto pSpecific = static_cast<const xiiAbstractArrayProperty*>(pProp);
        // Delete old values
        if (pProp->GetFlags().AreAllSet(xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner))
        {
          const xiiInt32 iCloneCount = (xiiInt32)pSpecific->GetCount(pClone);
          for (xiiInt32 i = iCloneCount - 1; i >= 0; --i)
          {
            void* pOldSubClone = nullptr;
            pSpecific->GetValue(pClone, i, &pOldSubClone);
            pSpecific->Remove(pClone, i);
            if (pOldSubClone)
            {
              xiiReflectionUtils::DeleteObject(pOldSubClone, pProp);
            }
          }
        }

        const xiiUInt32 uiCount = pSpecific->GetCount(pObject);
        pSpecific->SetCount(pClone, uiCount);
        if (pSpecific->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        {
          for (xiiUInt32 i = 0; i < uiCount; ++i)
          {
            vTemp                  = xiiReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
            void* pRefrencedObject = vTemp.ConvertTo<void*>();
            if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner) && pRefrencedObject)
            {
              pRefrencedObject = xiiReflectionSerializer::Clone(pRefrencedObject, pPropType);
              vTemp            = xiiVariant(pRefrencedObject, pPropType);
            }
            xiiReflectionUtils::SetArrayPropertyValue(pSpecific, pClone, i, vTemp);
          }
        }
        else
        {
          if (bIsValueType)
          {
            for (xiiUInt32 i = 0; i < uiCount; ++i)
            {
              vTemp = xiiReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
              xiiReflectionUtils::SetArrayPropertyValue(pSpecific, pClone, i, vTemp);
            }
          }
          else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class) && pPropType->GetAllocator()->CanAllocate())
          {
            void* pSubObject = pPropType->GetAllocator()->Allocate<void>();

            for (xiiUInt32 i = 0; i < uiCount; ++i)
            {
              pSpecific->GetValue(pObject, i, pSubObject);
              pSpecific->SetValue(pClone, i, pSubObject);
            }

            pPropType->GetAllocator()->Deallocate(pSubObject);
          }
        }
      }
      break;
      case xiiPropertyCategory::Set:
      {
        auto pSpecific = static_cast<const xiiAbstractSetProperty*>(pProp);

        // Delete old values
        if (pProp->GetFlags().AreAllSet(xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner))
        {
          xiiHybridArray<xiiVariant, 16> keys;
          pSpecific->GetValues(pClone, keys);
          pSpecific->Clear(pClone);
          for (xiiVariant& value : keys)
          {
            void* pOldClone = value.ConvertTo<void*>();
            if (pOldClone)
            {
              xiiReflectionUtils::DeleteObject(pOldClone, pProp);
            }
          }
        }
        pSpecific->Clear(pClone);

        xiiHybridArray<xiiVariant, 16> values;
        pSpecific->GetValues(pObject, values);


        if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        {
          for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
          {
            void* pRefrencedObject = values[i].ConvertTo<void*>();
            if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner) && pRefrencedObject)
            {
              pRefrencedObject = xiiReflectionSerializer::Clone(pRefrencedObject, pPropType);
            }
            vTemp = xiiVariant(pRefrencedObject, pPropType);
            xiiReflectionUtils::InsertSetPropertyValue(pSpecific, pClone, vTemp);
          }
        }
        else if (bIsValueType)
        {
          for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
          {
            xiiReflectionUtils::InsertSetPropertyValue(pSpecific, pClone, values[i]);
          }
        }
      }
      break;
      case xiiPropertyCategory::Map:
      {
        auto pSpecific = static_cast<const xiiAbstractMapProperty*>(pProp);

        // Delete old values
        if (pProp->GetFlags().AreAllSet(xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner))
        {
          xiiHybridArray<xiiString, 16> keys;
          pSpecific->GetKeys(pClone, keys);
          for (const xiiString& sKey : keys)
          {
            xiiVariant value     = xiiReflectionUtils::GetMapPropertyValue(pSpecific, pClone, sKey);
            void*      pOldClone = value.ConvertTo<void*>();
            pSpecific->Remove(pClone, sKey);
            if (pOldClone)
            {
              xiiReflectionUtils::DeleteObject(pOldClone, pProp);
            }
          }
        }
        pSpecific->Clear(pClone);

        xiiHybridArray<xiiString, 16> keys;
        pSpecific->GetKeys(pObject, keys);

        for (xiiUInt32 i = 0; i < keys.GetCount(); ++i)
        {
          if (bIsValueType || (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner)))
          {
            xiiVariant value = xiiReflectionUtils::GetMapPropertyValue(pSpecific, pObject, keys[i]);
            xiiReflectionUtils::SetMapPropertyValue(pSpecific, pClone, keys[i], value);
          }
          else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class))
          {
            if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
            {
              void* pValue = nullptr;
              pSpecific->GetValue(pObject, keys[i], &pValue);
              pValue = xiiReflectionSerializer::Clone(pValue, pPropType);
              pSpecific->Insert(pClone, keys[i], &pValue);
            }
            else
            {
              if (pPropType->GetAllocator()->CanAllocate())
              {
                void* pValue = pPropType->GetAllocator()->Allocate<void>();
                XII_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(pValue););
                XII_VERIFY(pSpecific->GetValue(pObject, keys[i], pValue), "Previously retrieved key does not exist.");
                pSpecific->Insert(pClone, keys[i], pValue);
              }
              else
              {
                xiiLog::Error("The property '{0}' can not be cloned as the type '{1}' cannot be allocated.", pProp->GetPropertyName(), pPropType->GetTypeName());
              }
            }
          }
        }
      }
      break;
      default:
        break;
    }
  }

  static void CloneProperties(const void* pObject, void* pClone, const xiiRTTI* pType)
  {
    if (pType->GetParentType())
    {
      CloneProperties(pObject, pClone, pType->GetParentType());
    }

    for (auto* pProp : pType->GetProperties())
    {
      CloneProperty(pObject, pClone, pProp);
    }
  }
} // namespace

void* xiiReflectionSerializer::Clone(const void* pObject, const xiiRTTI* pType)
{
  if (!pObject)
    return nullptr;

  XII_ASSERT_DEV(pType != nullptr, "invalid type.");
  if (pType->IsDerivedFrom<xiiReflectedClass>())
  {
    const xiiReflectedClass* pRefObject = static_cast<const xiiReflectedClass*>(pObject);
    pType                               = pRefObject->GetDynamicRTTI();
  }

  XII_ASSERT_DEV(pType->GetAllocator()->CanAllocate(), "The type '{0}' can't be cloned!", pType->GetTypeName());
  void* pClone = pType->GetAllocator()->Allocate<void>();
  CloneProperties(pObject, pClone, pType);
  return pClone;
}


void xiiReflectionSerializer::Clone(const void* pObject, void* pClone, const xiiRTTI* pType)
{
  XII_ASSERT_DEV(pObject && pClone && pType, "invalid type.");
  if (pType->IsDerivedFrom<xiiReflectedClass>())
  {
    const xiiReflectedClass* pRefObject = static_cast<const xiiReflectedClass*>(pObject);
    pType                               = pRefObject->GetDynamicRTTI();
    XII_ASSERT_DEV(pType == static_cast<xiiReflectedClass*>(pClone)->GetDynamicRTTI(), "Object '{0}' and clone '{1}' have mismatching types!", pType->GetTypeName(), static_cast<xiiReflectedClass*>(pClone)->GetDynamicRTTI()->GetTypeName());
  }

  CloneProperties(pObject, pClone, pType);
}

XII_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_ReflectionSerializer);
