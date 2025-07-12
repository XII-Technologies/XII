#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/GUI/DynamicDefaultStateProvider.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

xiiSharedPtr<xiiDefaultStateProvider> xiiDynamicDefaultStateProvider::CreateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp)
{
  if (pProp)
  {
    auto* pAttrib = pProp->GetAttributeByType<xiiDynamicDefaultValueAttribute>();
    if (pAttrib && !pAttrib->GetClassProperty().IsEmpty())
    {
      return XII_DEFAULT_NEW(xiiDynamicDefaultStateProvider, pAccessor, pObject, pObject, pObject, pProp, 0);
    }
  }

  xiiInt32 iRootDepth = 0;
  if (pProp)
    iRootDepth += 1;

  const xiiDocumentObject* pCurrentObject = pObject;
  while (pCurrentObject)
  {
    const xiiAbstractProperty* pParentProp = pCurrentObject->GetParentPropertyType();
    if (!pParentProp)
      return nullptr;

    const auto* pAttrib = pParentProp->GetAttributeByType<xiiDynamicDefaultValueAttribute>();
    if (pAttrib)
    {
      iRootDepth += 1;
      return XII_DEFAULT_NEW(xiiDynamicDefaultStateProvider, pAccessor, pObject, pCurrentObject, pCurrentObject->GetParent(), pParentProp, iRootDepth);
    }
    iRootDepth += 2;
    pCurrentObject = pCurrentObject->GetParent();
  }
  return nullptr;
}

xiiDynamicDefaultStateProvider::xiiDynamicDefaultStateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiDocumentObject* pClassObject, const xiiDocumentObject* pRootObject, const xiiAbstractProperty* pRootProp, xiiInt32 iRootDepth) :
  m_pObject(pObject), m_pClassObject(pClassObject), m_pRootObject(pRootObject), m_pRootProp(pRootProp), m_iRootDepth(iRootDepth)
{
  m_pAttrib = m_pRootProp->GetAttributeByType<xiiDynamicDefaultValueAttribute>();
  XII_ASSERT_DEBUG(m_pAttrib, "xiiDynamicDefaultStateProvider was created for a property that does not have the xiiDynamicDefaultValueAttribute.");

  m_pClassType = xiiRTTI::FindTypeByName(m_pAttrib->GetClassType());
  XII_ASSERT_DEBUG(m_pClassType, "The dynamic meta data class type '{0}' does not exist", m_pAttrib->GetClassType());

  m_pClassSourceProp = m_pRootObject->GetType()->FindPropertyByName(m_pAttrib->GetClassSource());
  XII_ASSERT_DEBUG(m_pClassSourceProp, "The dynamic meta data class source '{0}' does not exist on type '{1}'", m_pAttrib->GetClassSource(), m_pRootObject->GetType()->GetTypeName());

  const bool bHasProperty = !m_pAttrib->GetClassProperty().IsEmpty();
  if (!bHasProperty)
  {
    XII_ASSERT_DEBUG(m_pRootProp->GetCategory() == xiiPropertyCategory::Member, "xiiDynamicDefaultValueAttribute must be on a member property if no ClassProperty is given.");
  }
  else
  {
    m_pClassProperty = m_pClassType->FindPropertyByName(m_pAttrib->GetClassProperty());

    XII_ASSERT_DEBUG(m_pClassProperty, "The dynamic meta data class type '{0}' does not have a property named '{1}'", m_pAttrib->GetClassType(), m_pAttrib->GetClassProperty());
  }
}

xiiInt32 xiiDynamicDefaultStateProvider::GetRootDepth() const
{
  return m_iRootDepth;
}

xiiColorGammaUB xiiDynamicDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return xiiColorGammaUB(0, 0, 0, 0);
}

xiiVariant xiiDynamicDefaultStateProvider::GetDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  const bool bIsValueType = xiiReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags);

  if (const xiiReflectedClass* pMeta = GetMetaInfo(pAccessor))
  {
    xiiPropertyPath propertyPath;
    if (CreatePath(pAccessor, pMeta, propertyPath, pObject, pProp, index).Failed())
    {
      return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
    }

    xiiVariant defaultValue;
    xiiResult  res = propertyPath.ReadProperty(const_cast<xiiReflectedClass*>(pMeta), *pMeta->GetDynamicRTTI(), [&](void* pLeaf, const xiiRTTI& type, const xiiAbstractProperty* pNativeProp, const xiiVariant& index) {
      XII_ASSERT_DEBUG(pProp->GetCategory() == pNativeProp->GetCategory(), "While properties don't need to match exactly, they need to be of the same category and type.");

      switch (pNativeProp->GetCategory())
      {
        case xiiPropertyCategory::Member:
          defaultValue = xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(pNativeProp), pLeaf);
          break;
        case xiiPropertyCategory::Array:
        {
          xiiVariant currentValue;
          pAccessor->GetValue(pObject, pProp, currentValue).LogFailure();
          const xiiVariantArray& currentArray = currentValue.Get<xiiVariantArray>();

          auto* pArrayProp = static_cast<const xiiAbstractArrayProperty*>(pNativeProp);
          if (!index.IsValid())
          {
            xiiVariantArray varArray;
            varArray.SetCount(pArrayProp->GetCount(pLeaf));
            for (xiiUInt32 i = 0; i < pArrayProp->GetCount(pLeaf); i++)
            {
              if (bIsValueType)
              {
                varArray[i] = xiiReflectionUtils::GetArrayPropertyValue(pArrayProp, pLeaf, i);
              }
              else
              {
                // We don't have any guid on the native object. Thus we just match the count basically and fill everything we can't find on our current object with 'nullptr', i.e. an invalid guid.
                if (i < currentArray.GetCount())
                {
                  varArray[i] = currentArray[i];
                }
                else
                {
                  varArray[i] = xiiUuid();
                }
              }
            }
            defaultValue = std::move(varArray);
          }
          else
          {
            if (bIsValueType)
            {
              defaultValue = xiiReflectionUtils::GetArrayPropertyValue(pArrayProp, pLeaf, index.ConvertTo<xiiInt32>());
            }
            else
            {
              xiiUInt32 iIndex = index.ConvertTo<xiiUInt32>();
              if (iIndex < currentArray.GetCount())
              {
                defaultValue = currentArray[iIndex];
              }
              else
              {
                defaultValue = xiiUuid();
              }
            }
          }
        }
        break;
        case xiiPropertyCategory::Map:
        {
          auto* pMapProp = static_cast<const xiiAbstractMapProperty*>(pNativeProp);

          xiiVariant currentValue;
          pAccessor->GetValue(pObject, pProp, currentValue).LogFailure();
          const xiiVariantDictionary& currentDict = currentValue.Get<xiiVariantDictionary>();

          if (!index.IsValid())
          {
            xiiHybridArray<xiiString, 16> keys;
            pMapProp->GetKeys(pLeaf, keys);

            xiiVariantDictionary varDict;
            for (auto& key : keys)
            {
              if (bIsValueType)
              {
                varDict.Insert(key, xiiReflectionUtils::GetMapPropertyValue(pMapProp, pLeaf, key));
              }
              else
              {
                if (auto* pValue = currentDict.GetValue(key))
                {
                  varDict.Insert(key, *pValue);
                }
                else
                {
                  varDict.Insert(key, xiiUuid());
                }
              }
            }
            defaultValue = std::move(varDict);
          }
          else
          {
            if (bIsValueType)
            {
              defaultValue = xiiReflectionUtils::GetMapPropertyValue(pMapProp, pLeaf, index.Get<xiiString>());
            }
            else
            {
              if (auto* pValue = currentDict.GetValue(index.Get<xiiString>()))
              {
                defaultValue = *pValue;
              }
              else
              {
                defaultValue = xiiUuid();
              }
            }
          }
        }
        break;

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    });

    if (res.Succeeded())
    {
      if (!DoesVariantMatchProperty(defaultValue, pProp, index))
      {
        xiiLog::Error("Default value '{}' does not match property '{}' at index '{}'", defaultValue, pProp->GetPropertyName(), index);
      }
      else
      {
        return defaultValue;
      }
    }
  }
  return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
}

const xiiReflectedClass* xiiDynamicDefaultStateProvider::GetMetaInfo(xiiObjectAccessorBase* pAccessor) const
{
  xiiVariant value;
  if (pAccessor->GetValue(m_pRootObject, m_pClassSourceProp, value).Succeeded())
  {
    if (value.IsA<xiiString>())
    {
      const auto& sValue = value.Get<xiiString>();
      if (const auto asset = xiiAssetCurator::GetSingleton()->FindSubAsset(sValue))
      {
        return asset->m_pAssetInfo->m_Info->GetMetaInfo(m_pClassType);
      }
    }
    else if (value.IsA<xiiStringView>())
    {
      const auto& sValue = value.Get<xiiStringView>();
      if (const auto asset = xiiAssetCurator::GetSingleton()->FindSubAsset(sValue))
      {
        return asset->m_pAssetInfo->m_Info->GetMetaInfo(m_pClassType);
      }
    }
  }

  return nullptr;
}

const xiiResult xiiDynamicDefaultStateProvider::CreatePath(xiiObjectAccessorBase* pAccessor, const xiiReflectedClass* pMeta, xiiPropertyPath& propertyPath, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  xiiObjectPropertyPathContext pathContext = {m_pClassProperty ? m_pRootObject : m_pClassObject, pAccessor, "Children"};

  xiiPropertyReference ref;
  ref.m_Object    = pObject->GetGuid();
  ref.m_pProperty = pProp;
  ref.m_Index     = index;

  xiiStringBuilder sPropPath;
  xiiObjectPropertyPath::CreatePropertyPath(pathContext, ref, sPropPath).LogFailure();
  if (m_pClassProperty)
  {
    sPropPath.ReplaceFirst(m_pRootProp->GetPropertyName(), m_pAttrib->GetClassProperty());
  }

  return propertyPath.InitializeFromPath(*pMeta->GetDynamicRTTI(), sPropPath);
}

xiiStatus xiiDynamicDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDeque<xiiAbstractGraphDiffOperation>& out_diff)
{
  if (const xiiReflectedClass* pMeta = GetMetaInfo(pAccessor))
  {
    xiiPropertyPath propertyPath;
    if (CreatePath(pAccessor, pMeta, propertyPath, pObject, pProp).Failed())
    {
      return xiiStatus(xiiFmt("Failed to find root object in object graph"));
    }

    xiiAbstractObjectGraph prefabSubGraph;
    xiiAbstractObjectNode* pPrefabSubRoot = nullptr;
    {
      // Create a graph of the native object, skipping all other properties except for the container in question.
      xiiRttiConverterContext context;
      xiiString               sRootPropertyName = pProp->GetPropertyName();
      // If we are dealing with an attributed container and pObject is its parent, then the root container property name can differ between the meta info and the target object so we have to rename it later to make the two graphs match.
      if (m_pClassProperty && pObject == m_pRootObject)
      {
        sRootPropertyName = m_pAttrib->GetClassProperty();
      }

      void*                  pNativeRootObject = nullptr;
      xiiRttiConverterWriter rttiConverter(&prefabSubGraph, &context, [&](const void* pObject, const xiiAbstractProperty* pCurrentProp) {
        if (pNativeRootObject == pObject && pCurrentProp->GetPropertyName() != sRootPropertyName)
          return false;
        return true;
      });

      auto WriteObject = [&](void* pLeafObject, const xiiRTTI& leafType, const xiiAbstractProperty* pLeafProp, const xiiVariant& index) {
        pNativeRootObject = pLeafObject;
        context.RegisterObject(pObject->GetGuid(), &leafType, pLeafObject);
        pPrefabSubRoot = rttiConverter.AddObjectToGraph(&leafType, pLeafObject);
        pPrefabSubRoot->RenameProperty(sRootPropertyName, pProp->GetPropertyName());
      };

      xiiVariant defaultValue;
      xiiResult  res = propertyPath.ReadProperty(const_cast<xiiReflectedClass*>(pMeta), *pMeta->GetDynamicRTTI(), WriteObject);
      if (res.Failed())
      {
        return xiiStatus(xiiFmt("Failed to find root object in object graph"));
      }
    }

    // Create graph from current object with only the container to be reverted present.
    xiiAbstractObjectGraph instanceSubGraph;
    xiiAbstractObjectNode* pInstanceSubRoot = nullptr;
    {
      xiiDocumentObjectConverterWriter writer(&instanceSubGraph, pObject->GetDocumentObjectManager(), [pRootObject = pObject, pRootProp = pProp](const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) {
        if (pObject == pRootObject && pProp != pRootProp)
          return false;
        return true;
      });
      pInstanceSubRoot = writer.AddObjectToGraph(pObject);
    }

    // Make the native graph match the guids of pObject graph.
    pPrefabSubRoot->SetType(pInstanceSubRoot->GetType());
    prefabSubGraph.ReMapNodeGuidsToMatchGraph(pPrefabSubRoot, instanceSubGraph, pInstanceSubRoot);
    prefabSubGraph.CreateDiffWithBaseGraph(instanceSubGraph, out_diff);
    return XII_SUCCESS;
  }
  return superPtr[0]->CreateRevertContainerDiff(superPtr.GetSubArray(1), pAccessor, pObject, pProp, out_diff);
}
