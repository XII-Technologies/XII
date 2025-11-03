#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

xiiAbstractObjectNode* xiiDocumentObjectConverterWriter::AddObjectToGraph(const xiiDocumentObject* pObject, xiiStringView sNodeName)
{
  xiiAbstractObjectNode* pNode = AddSubObjectToGraph(pObject, sNodeName);

  while (!m_QueuedObjects.IsEmpty())
  {
    auto itCur = m_QueuedObjects.GetIterator();

    AddSubObjectToGraph(itCur.Key(), nullptr);

    m_QueuedObjects.Remove(itCur);
  }

  return pNode;
}

void xiiDocumentObjectConverterWriter::AddProperty(xiiAbstractObjectNode* pNode, const xiiAbstractProperty* pProp, const xiiDocumentObject* pObject)
{
  if (m_Filter.IsValid() && !m_Filter(pObject, pProp))
    return;

  const xiiRTTI* pPropType    = pProp->GetSpecificType();
  const bool     bIsValueType = xiiReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case xiiPropertyCategory::Member:
    {
      if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
      {
        if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
        {
          const xiiUuid guid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).Get<xiiUuid>();

          pNode->AddProperty(pProp->GetPropertyName(), guid);
          if (guid.IsValid())
            m_QueuedObjects.Insert(m_pManager->GetObject(guid));
        }
        else
        {
          pNode->AddProperty(pProp->GetPropertyName(), pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()));
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
        {
          xiiStringBuilder sTemp;
          xiiReflectionUtils::EnumerationToString(pPropType, pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).ConvertTo<xiiInt64>(), sTemp);
          pNode->AddProperty(pProp->GetPropertyName(), sTemp.GetData());
        }
        else if (bIsValueType)
        {
          pNode->AddProperty(pProp->GetPropertyName(), pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()));
        }
        else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class))
        {
          const xiiUuid guid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).Get<xiiUuid>();
          XII_ASSERT_DEV(guid.IsValid(), "Embedded class cannot be null.");
          pNode->AddProperty(pProp->GetPropertyName(), guid);
          m_QueuedObjects.Insert(m_pManager->GetObject(guid));
        }
      }
    }

    break;

    case xiiPropertyCategory::Array:
    case xiiPropertyCategory::Set:
    {
      const xiiInt32 iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      XII_ASSERT_DEV(iCount >= 0, "Invalid array property size {0}", iCount);

      xiiVariantArray values;
      values.SetCount(iCount);

      for (xiiInt32 i = 0; i < iCount; ++i)
      {
        values[i] = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i);
        if (!bIsValueType)
        {
          m_QueuedObjects.Insert(m_pManager->GetObject(values[i].Get<xiiUuid>()));
        }
      }
      pNode->AddProperty(pProp->GetPropertyName(), values);
    }
    break;
    case xiiPropertyCategory::Map:
    {
      const xiiInt32 iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      XII_ASSERT_DEV(iCount >= 0, "Invalid map property size {0}", iCount);

      xiiVariantDictionary values;
      values.Reserve(iCount);
      xiiHybridArray<xiiVariant, 16> keys;
      pObject->GetTypeAccessor().GetKeys(pProp->GetPropertyName(), keys);

      for (const xiiVariant& key : keys)
      {
        xiiVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), key);
        values.Insert(key.Get<xiiString>(), value);
        if (!bIsValueType)
        {
          m_QueuedObjects.Insert(m_pManager->GetObject(value.Get<xiiUuid>()));
        }
      }
      pNode->AddProperty(pProp->GetPropertyName(), values);
    }
    break;
    case xiiPropertyCategory::Constant:
      // Nothing to do here.
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void xiiDocumentObjectConverterWriter::AddProperties(xiiAbstractObjectNode* pNode, const xiiDocumentObject* pObject)
{
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  pObject->GetTypeAccessor().GetType()->GetAllProperties(properties);

  for (const auto* pProp : properties)
  {
    AddProperty(pNode, pProp, pObject);
  }
}

xiiAbstractObjectNode* xiiDocumentObjectConverterWriter::AddSubObjectToGraph(const xiiDocumentObject* pObject, xiiStringView sNodeName)
{
  xiiAbstractObjectNode* pNode = m_pGraph->AddNode(pObject->GetGuid(), pObject->GetType()->GetTypeName(), pObject->GetType()->GetTypeVersion(), sNodeName);
  AddProperties(pNode, pObject);
  return pNode;
}

xiiDocumentObjectConverterReader::xiiDocumentObjectConverterReader(const xiiAbstractObjectGraph* pGraph, xiiDocumentObjectManager* pManager, Mode mode)
{
  m_pManager               = pManager;
  m_pGraph                 = pGraph;
  m_Mode                   = mode;
  m_uiUnknownTypeInstances = 0;
}

xiiDocumentObject* xiiDocumentObjectConverterReader::CreateObjectFromNode(const xiiAbstractObjectNode* pNode)
{
  xiiDocumentObject* pObject = nullptr;
  const xiiRTTI*     pType   = xiiRTTI::FindTypeByName(pNode->GetType());
  if (pType)
  {
    pObject = m_pManager->CreateObject(pType, pNode->GetGuid());
  }
  else
  {
    if (!m_UnknownTypes.Contains(pNode->GetType()))
    {
      xiiLog::Error("Cannot create node of unknown type '{0}'.", pNode->GetType());
      m_UnknownTypes.Insert(pNode->GetType());
    }
    m_uiUnknownTypeInstances++;
  }
  return pObject;
}

void xiiDocumentObjectConverterReader::AddObject(xiiDocumentObject* pObject, xiiDocumentObject* pParent, xiiStringView sParentProperty, xiiVariant index)
{
  XII_ASSERT_DEV(pObject && pParent, "Need to have valid objects to add them to the document");
  if (m_Mode == xiiDocumentObjectConverterReader::Mode::CreateAndAddToDocument && pParent->GetDocumentObjectManager()->GetObject(pParent->GetGuid()))
  {
    m_pManager->AddObject(pObject, pParent, sParentProperty, index);
  }
  else
  {
    pParent->InsertSubObject(pObject, sParentProperty, index);
  }
}

void xiiDocumentObjectConverterReader::ApplyPropertiesToObject(const xiiAbstractObjectNode* pNode, xiiDocumentObject* pObject)
{
  // XII_ASSERT_DEV(pObject->GetChildren().GetCount() == 0, "Can only apply properties to empty objects!");
  xiiHybridArray<const xiiAbstractProperty*, 32> properties;
  pObject->GetTypeAccessor().GetType()->GetAllProperties(properties);

  for (auto* pProp : properties)
  {
    auto* pOtherProp = pNode->FindProperty(pProp->GetPropertyName());
    if (pOtherProp == nullptr)
      continue;

    ApplyProperty(pObject, pProp, pOtherProp);
  }
}

void xiiDocumentObjectConverterReader::ApplyDiffToObject(xiiObjectAccessorBase* pObjectAccessor, const xiiDocumentObject* pObject, xiiDeque<xiiAbstractGraphDiffOperation>& ref_diff)
{
  xiiHybridArray<xiiAbstractGraphDiffOperation*, 4> change;

  for (auto& op : ref_diff)
  {
    if (op.m_Operation == xiiAbstractGraphDiffOperation::Op::PropertyChanged && pObject->GetGuid() == op.m_Node)
      change.PushBack(&op);
  }

  for (auto* op : change)
  {
    const xiiAbstractProperty* pProp = pObject->GetTypeAccessor().GetType()->FindPropertyByName(op->m_sProperty);
    if (!pProp)
      continue;

    ApplyDiff(pObjectAccessor, pObject, pProp, *op, ref_diff);
  }

  // Recurse into owned sub objects (old or new)
  for (const xiiDocumentObject* pSubObject : pObject->GetChildren())
  {
    ApplyDiffToObject(pObjectAccessor, pSubObject, ref_diff);
  }
}

void xiiDocumentObjectConverterReader::ApplyDiff(xiiObjectAccessorBase* pObjectAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiAbstractGraphDiffOperation& op, xiiDeque<xiiAbstractGraphDiffOperation>& diff)
{
  xiiStringBuilder sTemp;

  const bool bIsValueType = xiiReflectionUtils::IsValueType(pProp);

  auto NeedsToBeDeleted = [&diff](const xiiUuid& guid) -> bool {
    for (auto& op : diff)
    {
      if (op.m_Operation == xiiAbstractGraphDiffOperation::Op::NodeRemoved && guid == op.m_Node)
        return true;
    }
    return false;
  };
  auto NeedsToBeCreated = [&diff](const xiiUuid& guid) -> xiiAbstractGraphDiffOperation* {
    for (auto& op : diff)
    {
      if (op.m_Operation == xiiAbstractGraphDiffOperation::Op::NodeAdded && guid == op.m_Node)
        return &op;
    }
    return nullptr;
  };

  switch (pProp->GetCategory())
  {
    case xiiPropertyCategory::Member:
    {
      if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags) || bIsValueType)
      {
        const xiiVariantType::Enum memberType = xiiToolsReflectionUtils::GetStorageType(pProp);
        if (memberType != xiiVariantType::Invalid && bIsValueType && op.m_Value.GetType() != memberType)
        {
          op.m_Value = op.m_Value.ConvertTo(memberType);
        }

        pObjectAccessor->SetValue(pObject, pProp, op.m_Value).IgnoreResult();
      }
      else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class))
      {
        if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
        {
          if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
          {
            const xiiUuid oldGuid = pObjectAccessor->Get<xiiUuid>(pObject, pProp);
            const xiiUuid newGuid = op.m_Value.Get<xiiUuid>();
            if (oldGuid.IsValid())
            {
              if (NeedsToBeDeleted(oldGuid))
              {
                pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(oldGuid)).IgnoreResult();
              }
            }

            if (newGuid.IsValid())
            {
              if (xiiAbstractGraphDiffOperation* pCreate = NeedsToBeCreated(newGuid))
              {
                pObjectAccessor->AddObject(pObject, pProp, xiiVariant(), xiiRTTI::FindTypeByName(pCreate->m_sProperty), pCreate->m_Node).IgnoreResult();
              }

              const xiiDocumentObject* pChild = pObject->GetChild(newGuid);
              XII_ASSERT_DEV(pChild != nullptr, "References child object does not exist!");
            }
          }
          else
          {
            pObjectAccessor->SetValue(pObject, pProp, op.m_Value).IgnoreResult();
          }
        }
        else
        {
          // Noting to do here, value cannot change
        }
      }
      break;
    }
    case xiiPropertyCategory::Array:
    case xiiPropertyCategory::Set:
    {
      const xiiVariantArray& values        = op.m_Value.Get<xiiVariantArray>();
      xiiInt32               iCurrentCount = pObjectAccessor->GetCount(pObject, pProp);
      if (bIsValueType || (pProp->GetFlags().IsAnySet(xiiPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner)))
      {
        for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
        {
          if (i < (xiiUInt32)iCurrentCount)
            pObjectAccessor->SetValue(pObject, pProp, values[i], i).IgnoreResult();
          else
            pObjectAccessor->InsertValue(pObject, pProp, values[i], i).IgnoreResult();
        }
        for (xiiInt32 i = iCurrentCount - 1; i >= (xiiInt32)values.GetCount(); --i)
        {
          pObjectAccessor->RemoveValue(pObject, pProp, i).IgnoreResult();
        }
      }
      else // Class
      {
        const xiiInt32 iCurrentCount2 = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());

        xiiHybridArray<xiiVariant, 16> currentValues;
        pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), currentValues);
        for (xiiInt32 i = iCurrentCount2 - 1; i >= 0; --i)
        {
          if (NeedsToBeDeleted(currentValues[i].Get<xiiUuid>()))
          {
            pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(currentValues[i].Get<xiiUuid>())).IgnoreResult();
          }
        }

        for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
        {
          if (xiiAbstractGraphDiffOperation* pCreate = NeedsToBeCreated(values[i].Get<xiiUuid>()))
          {
            pObjectAccessor->AddObject(pObject, pProp, i, xiiRTTI::FindTypeByName(pCreate->m_sProperty), pCreate->m_Node).IgnoreResult();
          }
          else
          {
            pObjectAccessor->MoveObject(pObjectAccessor->GetObject(values[i].Get<xiiUuid>()), pObject, pProp, i).IgnoreResult();
          }
        }
      }
      break;
    }
    case xiiPropertyCategory::Map:
    {
      const xiiVariantDictionary&    values = op.m_Value.Get<xiiVariantDictionary>();
      xiiHybridArray<xiiVariant, 16> keys;
      XII_VERIFY(pObjectAccessor->GetKeys(pObject, pProp, keys).Succeeded(), "Property is not a map, getting keys failed.");

      if (bIsValueType || (pProp->GetFlags().IsAnySet(xiiPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner)))
      {
        for (const xiiVariant& key : keys)
        {
          const xiiString& sKey = key.Get<xiiString>();
          if (!values.Contains(sKey))
          {
            XII_VERIFY(pObjectAccessor->RemoveValue(pObject, pProp, key).Succeeded(), "RemoveValue failed.");
          }
        }
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          xiiVariant variantKey(it.Key());
          if (keys.Contains(variantKey))
            pObjectAccessor->SetValue(pObject, pProp, it.Value(), variantKey).IgnoreResult();
          else
            pObjectAccessor->InsertValue(pObject, pProp, it.Value(), variantKey).IgnoreResult();
        }
      }
      else // Class
      {
        for (const xiiVariant& key : keys)
        {
          xiiVariant value;
          XII_VERIFY(pObjectAccessor->GetValue(pObject, pProp, value, key).Succeeded(), "");
          if (NeedsToBeDeleted(value.Get<xiiUuid>()))
          {
            pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(value.Get<xiiUuid>())).IgnoreResult();
          }
        }
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          const xiiVariant& value = it.Value();
          xiiVariant        variantKey(it.Key());
          if (xiiAbstractGraphDiffOperation* pCreate = NeedsToBeCreated(value.Get<xiiUuid>()))
          {
            pObjectAccessor->AddObject(pObject, pProp, variantKey, xiiRTTI::FindTypeByName(pCreate->m_sProperty), pCreate->m_Node).IgnoreResult();
          }
          else
          {
            pObjectAccessor->MoveObject(pObjectAccessor->GetObject(value.Get<xiiUuid>()), pObject, pProp, variantKey).IgnoreResult();
          }
        }
      }
      break;
    }

    case xiiPropertyCategory::Function:
    case xiiPropertyCategory::Constant:
      break; // nothing to do
  }
}

void xiiDocumentObjectConverterReader::ApplyProperty(xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiAbstractObjectNode::Property* pSource)
{
  xiiStringBuilder sTemp;

  const bool bIsValueType = xiiReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case xiiPropertyCategory::Member:
    {
      if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
      {
        if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
        {
          if (pSource->m_Value.IsA<xiiUuid>())
          {
            const xiiUuid guid = pSource->m_Value.Get<xiiUuid>();
            if (guid.IsValid())
            {
              auto* pSubNode = m_pGraph->GetNode(guid);
              XII_ASSERT_DEV(pSubNode != nullptr, "invalid document");

              if (auto* pSubObject = CreateObjectFromNode(pSubNode))
              {
                ApplyPropertiesToObject(pSubNode, pSubObject);
                AddObject(pSubObject, pObject, pProp->GetPropertyName(), xiiVariant());
              }
            }
          }
        }
        else
        {
          pObject->GetTypeAccessor().SetValue(pProp->GetPropertyName(), pSource->m_Value);
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags) || bIsValueType)
        {
          pObject->GetTypeAccessor().SetValue(pProp->GetPropertyName(), pSource->m_Value);
        }
        else if (pSource->m_Value.IsA<xiiUuid>()) // xiiPropertyFlags::Class
        {
          const xiiUuid& nodeGuid = pSource->m_Value.Get<xiiUuid>();
          if (nodeGuid.IsValid())
          {
            const xiiUuid      subObjectGuid        = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).Get<xiiUuid>();
            xiiDocumentObject* pEmbeddedClassObject = pObject->GetChild(subObjectGuid);
            XII_ASSERT_DEV(pEmbeddedClassObject != nullptr, "CreateObject should have created all embedded classes!");
            auto* pSubNode = m_pGraph->GetNode(nodeGuid);
            XII_ASSERT_DEV(pSubNode != nullptr, "invalid document");

            ApplyPropertiesToObject(pSubNode, pEmbeddedClassObject);
          }
        }
      }
      break;
    }
    case xiiPropertyCategory::Array:
    case xiiPropertyCategory::Set:
    {
      const xiiVariantArray& array         = pSource->m_Value.Get<xiiVariantArray>();
      const xiiInt32         iCurrentCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      if (bIsValueType || (pProp->GetFlags().IsAnySet(xiiPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner)))
      {
        for (xiiUInt32 i = 0; i < array.GetCount(); ++i)
        {
          if (i < (xiiUInt32)iCurrentCount)
          {
            pObject->GetTypeAccessor().SetValue(pProp->GetPropertyName(), array[i], i);
          }
          else
          {
            pObject->GetTypeAccessor().InsertValue(pProp->GetPropertyName(), i, array[i]);
          }
        }
        for (xiiInt32 i = iCurrentCount - 1; i >= (xiiInt32)array.GetCount(); i--)
        {
          pObject->GetTypeAccessor().RemoveValue(pProp->GetPropertyName(), i);
        }
      }
      else
      {
        for (xiiUInt32 i = 0; i < array.GetCount(); ++i)
        {
          const xiiUuid guid = array[i].Get<xiiUuid>();
          if (guid.IsValid())
          {
            auto* pSubNode = m_pGraph->GetNode(guid);
            XII_ASSERT_DEV(pSubNode != nullptr, "invalid document");

            if (i < (xiiUInt32)iCurrentCount)
            {
              // Overwrite existing object
              xiiUuid childGuid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i).ConvertTo<xiiUuid>();
              if (xiiDocumentObject* pSubObject = m_pManager->GetObject(childGuid))
              {
                ApplyPropertiesToObject(pSubNode, pSubObject);
              }
            }
            else
            {
              if (xiiDocumentObject* pSubObject = CreateObjectFromNode(pSubNode))
              {
                ApplyPropertiesToObject(pSubNode, pSubObject);
                AddObject(pSubObject, pObject, pProp->GetPropertyName(), -1);
              }
            }
          }
        }
        for (xiiInt32 i = iCurrentCount - 1; i >= (xiiInt32)array.GetCount(); i--)
        {
          XII_REPORT_FAILURE("Not implemented");
        }
      }
      break;
    }
    case xiiPropertyCategory::Map:
    {
      const xiiVariantDictionary&    values = pSource->m_Value.Get<xiiVariantDictionary>();
      xiiHybridArray<xiiVariant, 16> keys;
      pObject->GetTypeAccessor().GetKeys(pProp->GetPropertyName(), keys);

      if (bIsValueType || (pProp->GetFlags().IsAnySet(xiiPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner)))
      {
        for (const xiiVariant& key : keys)
        {
          pObject->GetTypeAccessor().RemoveValue(pProp->GetPropertyName(), key);
        }
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          pObject->GetTypeAccessor().InsertValue(pProp->GetPropertyName(), xiiVariant(it.Key()), it.Value());
        }
      }
      else
      {
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          const xiiVariant& value = it.Value();
          const xiiUuid     guid  = value.Get<xiiUuid>();

          const xiiVariant variantKey(it.Key());

          if (guid.IsValid())
          {
            auto* pSubNode = m_pGraph->GetNode(guid);
            XII_ASSERT_DEV(pSubNode != nullptr, "invalid document");
            if (xiiDocumentObject* pSubObject = CreateObjectFromNode(pSubNode))
            {
              ApplyPropertiesToObject(pSubNode, pSubObject);
              AddObject(pSubObject, pObject, pProp->GetPropertyName(), variantKey);
            }
          }
        }
      }
      break;
    }

    case xiiPropertyCategory::Function:
    case xiiPropertyCategory::Constant:
      break; // nothing to do
  }
}
