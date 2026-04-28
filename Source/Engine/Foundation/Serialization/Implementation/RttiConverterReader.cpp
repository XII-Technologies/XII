/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/VariantTypeRegistry.h>

xiiRttiConverterReader::xiiRttiConverterReader(const xiiAbstractObjectGraph* pGraph, xiiRttiConverterContext* pContext)
{
  m_pGraph   = pGraph;
  m_pContext = pContext;
}

xiiInternal::NewInstance<void> xiiRttiConverterReader::CreateObjectFromNode(const xiiAbstractObjectNode* pNode)
{
  const xiiRTTI* pRtti = m_pContext->FindTypeByName(pNode->GetType());
  if (pRtti == nullptr)
  {
    m_pContext->OnUnknownTypeError(pNode->GetType());
    return nullptr;
  }

  auto pObject = m_pContext->CreateObject(pNode->GetGuid(), pRtti);
  if (pObject)
  {
    ApplyPropertiesToObject(pNode, pRtti, pObject);
  }

  CallOnObjectCreated(pNode, pRtti, pObject);
  return pObject;
}

void xiiRttiConverterReader::ApplyPropertiesToObject(const xiiAbstractObjectNode* pNode, const xiiRTTI* pRtti, void* pObject)
{
  XII_ASSERT_DEBUG(pNode != nullptr, "Invalid node");

  if (pRtti->GetParentType() != nullptr)
  {
    ApplyPropertiesToObject(pNode, pRtti->GetParentType(), pObject);
  }

  for (auto* prop : pRtti->GetProperties())
  {
    auto* pOtherProp = pNode->FindProperty(prop->GetPropertyName());
    if (pOtherProp == nullptr)
      continue;

    ApplyProperty(pObject, prop, pOtherProp);
  }
}

void xiiRttiConverterReader::ApplyProperty(void* pObject, const xiiAbstractProperty* pProp, const xiiAbstractObjectNode::Property* pSource)
{
  const xiiRTTI* pPropType = pProp->GetSpecificType();

  if (pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
    return;

  const bool bIsValueType = xiiReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case xiiPropertyCategory::Member:
    {
      auto pSpecific = static_cast<const xiiAbstractMemberProperty*>(pProp);

      if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
      {
        if (!pSource->m_Value.IsA<xiiUuid>())
          return;

        xiiUuid guid             = pSource->m_Value.Get<xiiUuid>();
        void*   pRefrencedObject = nullptr;

        if (guid.IsValid())
        {
          if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
          {
            auto* pNode = m_pGraph->GetNode(guid);
            XII_ASSERT_DEV(pNode != nullptr, "node must exist");
            pRefrencedObject = CreateObjectFromNode(pNode);
            if (pRefrencedObject == nullptr)
            {
              // xiiLog::Error("Failed to set property '{0}', type could not be created!", pProp->GetPropertyName());
              return;
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
        }

        void* pOldObject = nullptr;
        pSpecific->GetValuePtr(pObject, &pOldObject);
        pSpecific->SetValuePtr(pObject, &pRefrencedObject);
        if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
        {
          xiiReflectionUtils::DeleteObject(pOldObject, pProp);
        }
      }
      else
      {
        if (bIsValueType || pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
        {
          xiiReflectionUtils::SetMemberPropertyValue(pSpecific, pObject, pSource->m_Value);
        }
        else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class))
        {
          if (!pSource->m_Value.IsA<xiiUuid>())
            return;

          void*         pDirectPtr = pSpecific->GetPropertyPointer(pObject);
          bool          bDelete    = false;
          const xiiUuid sourceGuid = pSource->m_Value.Get<xiiUuid>();

          if (pDirectPtr == nullptr)
          {
            bDelete    = true;
            pDirectPtr = m_pContext->CreateObject(sourceGuid, pPropType);
          }

          auto* pNode = m_pGraph->GetNode(sourceGuid);
          XII_ASSERT_DEV(pNode != nullptr, "node must exist");

          ApplyPropertiesToObject(pNode, pPropType, pDirectPtr);

          if (bDelete)
          {
            pSpecific->SetValuePtr(pObject, pDirectPtr);
            m_pContext->DeleteObject(sourceGuid);
          }
        }
      }
    }
    break;
    case xiiPropertyCategory::Array:
    {
      auto pSpecific = static_cast<const xiiAbstractArrayProperty*>(pProp);
      if (!pSource->m_Value.IsA<xiiVariantArray>())
        return;
      const xiiVariantArray& array = pSource->m_Value.Get<xiiVariantArray>();
      // Delete old values
      if (pProp->GetFlags().AreAllSet(xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner))
      {
        const xiiInt32 uiOldCount = (xiiInt32)pSpecific->GetCount(pObject);
        for (xiiInt32 i = uiOldCount - 1; i >= 0; --i)
        {
          void* pOldObject = nullptr;
          pSpecific->GetValue(pObject, i, &pOldObject);
          pSpecific->Remove(pObject, i);
          if (pOldObject)
          {
            xiiReflectionUtils::DeleteObject(pOldObject, pProp);
          }
        }
      }

      pSpecific->SetCount(pObject, array.GetCount());
      if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::Pointer))
      {
        for (xiiUInt32 i = 0; i < array.GetCount(); ++i)
        {
          if (!array[i].IsA<xiiUuid>())
            continue;
          xiiUuid guid             = array[i].Get<xiiUuid>();
          void*   pRefrencedObject = nullptr;
          if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
          {
            if (guid.IsValid())
            {
              auto* pNode = m_pGraph->GetNode(guid);
              XII_ASSERT_DEV(pNode != nullptr, "node must exist");
              pRefrencedObject = CreateObjectFromNode(pNode);
              if (pRefrencedObject == nullptr)
              {
                xiiLog::Error("Failed to set array property '{0}' element, type could not be created!", pProp->GetPropertyName());
                continue;
              }
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
          pSpecific->SetValue(pObject, i, &pRefrencedObject);
        }
      }
      else
      {
        if (bIsValueType)
        {
          for (xiiUInt32 i = 0; i < array.GetCount(); ++i)
          {
            xiiReflectionUtils::SetArrayPropertyValue(pSpecific, pObject, i, array[i]);
          }
        }
        else if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::Class))
        {
          xiiUuid temp = xiiUuid::MakeUuid();

          void* pValuePtr = m_pContext->CreateObject(temp, pPropType);

          for (xiiUInt32 i = 0; i < array.GetCount(); ++i)
          {
            if (!array[i].IsA<xiiUuid>())
              continue;

            const xiiUuid sourceGuid = array[i].Get<xiiUuid>();
            auto*         pNode      = m_pGraph->GetNode(sourceGuid);
            XII_ASSERT_DEV(pNode != nullptr, "node must exist");

            ApplyPropertiesToObject(pNode, pPropType, pValuePtr);
            pSpecific->SetValue(pObject, i, pValuePtr);
          }

          m_pContext->DeleteObject(temp);
        }
      }
    }
    break;
    case xiiPropertyCategory::Set:
    {
      auto pSpecific = static_cast<const xiiAbstractSetProperty*>(pProp);
      if (!pSource->m_Value.IsA<xiiVariantArray>())
        return;

      const xiiVariantArray& array = pSource->m_Value.Get<xiiVariantArray>();

      // Delete old values
      if (pProp->GetFlags().AreAllSet(xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner))
      {
        xiiHybridArray<xiiVariant, 16> keys;
        pSpecific->GetValues(pObject, keys);
        pSpecific->Clear(pObject);
        for (xiiVariant& value : keys)
        {
          void* pOldObject = value.ConvertTo<void*>();
          if (pOldObject)
          {
            xiiReflectionUtils::DeleteObject(pOldObject, pProp);
          }
        }
      }

      pSpecific->Clear(pObject);

      if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::Pointer))
      {
        for (xiiUInt32 i = 0; i < array.GetCount(); ++i)
        {
          if (!array[i].IsA<xiiUuid>())
            continue;

          xiiUuid guid             = array[i].Get<xiiUuid>();
          void*   pRefrencedObject = nullptr;
          if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
          {
            auto* pNode = m_pGraph->GetNode(guid);
            XII_ASSERT_DEV(pNode != nullptr, "node must exist");
            pRefrencedObject = CreateObjectFromNode(pNode);
            if (pRefrencedObject == nullptr)
            {
              xiiLog::Error("Failed to insert set element into property '{0}', type could not be created!", pProp->GetPropertyName());
              continue;
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
          pSpecific->Insert(pObject, &pRefrencedObject);
        }
      }
      else
      {
        if (bIsValueType)
        {
          for (xiiUInt32 i = 0; i < array.GetCount(); ++i)
          {
            xiiReflectionUtils::InsertSetPropertyValue(pSpecific, pObject, array[i]);
          }
        }
        else if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::Class))
        {
          xiiUuid temp = xiiUuid::MakeUuid();

          void* pValuePtr = m_pContext->CreateObject(temp, pPropType);

          for (xiiUInt32 i = 0; i < array.GetCount(); ++i)
          {
            if (!array[i].IsA<xiiUuid>())
              continue;

            const xiiUuid sourceGuid = array[i].Get<xiiUuid>();
            auto*         pNode      = m_pGraph->GetNode(sourceGuid);
            XII_ASSERT_DEV(pNode != nullptr, "node must exist");

            ApplyPropertiesToObject(pNode, pPropType, pValuePtr);
            pSpecific->Insert(pObject, pValuePtr);
          }

          m_pContext->DeleteObject(temp);
        }
      }
    }
    break;
    case xiiPropertyCategory::Map:
    {
      auto pSpecific = static_cast<const xiiAbstractMapProperty*>(pProp);
      if (!pSource->m_Value.IsA<xiiVariantDictionary>())
        return;

      const xiiVariantDictionary& dict = pSource->m_Value.Get<xiiVariantDictionary>();

      // Delete old values
      if (pProp->GetFlags().AreAllSet(xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner))
      {
        xiiHybridArray<xiiString, 16> keys;
        pSpecific->GetKeys(pObject, keys);
        for (const xiiString& sKey : keys)
        {
          xiiVariant value     = xiiReflectionUtils::GetMapPropertyValue(pSpecific, pObject, sKey);
          void*      pOldClone = value.ConvertTo<void*>();
          pSpecific->Remove(pObject, sKey);
          if (pOldClone)
          {
            xiiReflectionUtils::DeleteObject(pOldClone, pProp);
          }
        }
      }

      pSpecific->Clear(pObject);

      if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::Pointer))
      {
        for (auto it = dict.GetIterator(); it.IsValid(); ++it)
        {
          if (!it.Value().IsA<xiiUuid>())
            continue;

          xiiUuid guid             = it.Value().Get<xiiUuid>();
          void*   pRefrencedObject = nullptr;
          if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
          {
            if (guid.IsValid())
            {
              auto* pNode = m_pGraph->GetNode(guid);
              XII_ASSERT_DEV(pNode != nullptr, "node must exist");
              pRefrencedObject = CreateObjectFromNode(pNode);
              if (pRefrencedObject == nullptr)
              {
                xiiLog::Error("Failed to insert set element into property '{0}', type could not be created!", pProp->GetPropertyName());
                continue;
              }
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
          pSpecific->Insert(pObject, it.Key(), &pRefrencedObject);
        }
      }
      else
      {
        if (bIsValueType)
        {
          for (auto it = dict.GetIterator(); it.IsValid(); ++it)
          {
            xiiReflectionUtils::SetMapPropertyValue(pSpecific, pObject, it.Key(), it.Value());
          }
        }
        else if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::Class))
        {
          xiiUuid temp = xiiUuid::MakeUuid();

          void* pValuePtr = m_pContext->CreateObject(temp, pPropType);

          for (auto it = dict.GetIterator(); it.IsValid(); ++it)
          {
            if (!it.Value().IsA<xiiUuid>())
              continue;

            const xiiUuid sourceGuid = it.Value().Get<xiiUuid>();
            auto*         pNode      = m_pGraph->GetNode(sourceGuid);
            XII_ASSERT_DEV(pNode != nullptr, "node must exist");

            ApplyPropertiesToObject(pNode, pPropType, pValuePtr);
            pSpecific->Insert(pObject, it.Key(), pValuePtr);
          }

          m_pContext->DeleteObject(temp);
        }
      }
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void xiiRttiConverterReader::CallOnObjectCreated(const xiiAbstractObjectNode* pNode, const xiiRTTI* pRtti, void* pObject)
{
  auto functions = pRtti->GetFunctions();
  for (auto pFunc : functions)
  {
    // TODO: Make this compare faster
    if (pFunc->GetPropertyName().IsEqual("OnObjectCreated"))
    {
      xiiHybridArray<xiiVariant, 1> params;
      params.PushBack(xiiVariant(pNode));
      xiiVariant ret;
      pFunc->Execute(pObject, params, ret);
    }
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_RttiConverterReader);
