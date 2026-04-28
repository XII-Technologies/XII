/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/PrefabDefaultStateProvider.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

xiiSharedPtr<xiiDefaultStateProvider> xiiPrefabDefaultStateProvider::CreateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp)
{
  const auto* pMetaData      = pObject->GetDocumentObjectManager()->GetDocument()->m_DocumentObjectMetaData.Borrow();
  xiiInt32    iRootDepth     = 0;
  xiiUuid     rootObjectGuid = xiiPrefabUtils::GetPrefabRoot(pObject, *pMetaData, &iRootDepth);
  // The root depth is taken x2 because GetPrefabRoot counts the number of parent objects while xiiDefaultStateProvider expects to count the properties as well.
  iRootDepth *= 2;
  // If we construct this from a property scope, the root is an additional hop away as GetPrefabRoot counts from the parent object.
  if (pProp)
    iRootDepth += 1;

  if (rootObjectGuid.IsValid())
  {
    auto pMeta = pMetaData->BeginReadMetaData(rootObjectGuid);
    XII_SCOPE_EXIT(pMetaData->EndReadMetaData(););
    xiiUuid objectPrefabGuid = pObject->GetGuid();
    objectPrefabGuid.RevertCombinationWithSeed(pMeta->m_PrefabSeedGuid);
    const xiiAbstractObjectGraph* pGraph = xiiPrefabCache::GetSingleton()->GetCachedPrefabGraph(pMeta->m_CreateFromPrefab);
    if (pGraph)
    {
      if (pGraph->GetNode(objectPrefabGuid) != nullptr)
      {
        // The object was found in the prefab, we can thus use its prefab counterpart to provide a default state.
        return XII_DEFAULT_NEW(xiiPrefabDefaultStateProvider, rootObjectGuid, pMeta->m_CreateFromPrefab, pMeta->m_PrefabSeedGuid, iRootDepth);
      }
    }
  }
  return nullptr;
}

xiiPrefabDefaultStateProvider::xiiPrefabDefaultStateProvider(const xiiUuid& rootObjectGuid, const xiiUuid& createFromPrefab, const xiiUuid& prefabSeedGuid, xiiInt32 iRootDepth) :
  m_RootObjectGuid(rootObjectGuid), m_CreateFromPrefab(createFromPrefab), m_PrefabSeedGuid(prefabSeedGuid), m_iRootDepth(iRootDepth)
{
}

xiiInt32 xiiPrefabDefaultStateProvider::GetRootDepth() const
{
  return m_iRootDepth;
}

xiiColorGammaUB xiiPrefabDefaultStateProvider::GetBackgroundColor() const
{
  return xiiColorScheme::DarkUI(xiiColorScheme::Blue).WithAlpha(0.25f);
}

xiiVariant xiiPrefabDefaultStateProvider::GetDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  const bool bIsValueType = xiiReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags);

  const xiiAbstractObjectGraph* pGraph           = xiiPrefabCache::GetSingleton()->GetCachedPrefabGraph(m_CreateFromPrefab);
  xiiUuid                       objectPrefabGuid = pObject->GetGuid();
  objectPrefabGuid.RevertCombinationWithSeed(m_PrefabSeedGuid);
  if (pGraph)
  {
    bool       bValueFound  = true;
    xiiVariant defaultValue = xiiPrefabUtils::GetDefaultValue(*pGraph, objectPrefabGuid, pProp->GetPropertyName(), index, &bValueFound);
    if (!bValueFound)
    {
      return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
    }

    if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags) && defaultValue.IsA<xiiString>())
    {
      xiiInt64 iValue = 0;
      if (xiiReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), defaultValue.Get<xiiString>(), iValue))
      {
        defaultValue = iValue;
      }
      else
      {
        defaultValue = superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
      }
    }
    else if (!bIsValueType)
    {
      // For object references we need to reverse the object GUID mapping from prefab -> instance.
      switch (pProp->GetCategory())
      {
        case xiiPropertyCategory::Member:
        {
          xiiUuid& targetGuid = defaultValue.GetWritable<xiiUuid>();
          targetGuid.CombineWithSeed(m_PrefabSeedGuid);
        }
        break;
        case xiiPropertyCategory::Array:
        case xiiPropertyCategory::Set:
        {
          if (index.IsValid())
          {
            xiiUuid& targetGuid = defaultValue.GetWritable<xiiUuid>();
            targetGuid.CombineWithSeed(m_PrefabSeedGuid);
          }
          else
          {
            xiiVariantArray& defaultValueArray = defaultValue.GetWritable<xiiVariantArray>();
            for (xiiVariant& value : defaultValueArray)
            {
              xiiUuid& targetGuid = value.GetWritable<xiiUuid>();
              targetGuid.CombineWithSeed(m_PrefabSeedGuid);
            }
          }
        }
        break;
        case xiiPropertyCategory::Map:
        {
          if (index.IsValid())
          {
            xiiUuid& targetGuid = defaultValue.GetWritable<xiiUuid>();
            targetGuid.CombineWithSeed(m_PrefabSeedGuid);
          }
          else
          {
            xiiVariantDictionary& defaultValueDict = defaultValue.GetWritable<xiiVariantDictionary>();
            for (auto it : defaultValueDict)
            {
              xiiUuid& targetGuid = it.Value().GetWritable<xiiUuid>();
              targetGuid.CombineWithSeed(m_PrefabSeedGuid);
            }
          }
        }
        break;
        default:
          break;
      }
    }

    if (defaultValue.IsValid())
    {
      if (defaultValue.IsString() && pProp->GetAttributeByType<xiiGameObjectReferenceAttribute>())
      {
        // While pretty expensive this restores the default state of game object references which are stored as strings.
        xiiString sValue = defaultValue.ConvertTo<xiiString>();
        if (xiiConversionUtils::IsStringUuid(sValue))
        {
          xiiUuid guid = xiiConversionUtils::ConvertStringToUuid(sValue);
          guid.CombineWithSeed(m_PrefabSeedGuid);
          xiiStringBuilder sTemp;
          defaultValue = xiiConversionUtils::ToString(guid, sTemp).GetData();
        }
      }

      return defaultValue;
    }
  }
  return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp);
}

xiiStatus xiiPrefabDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDeque<xiiAbstractGraphDiffOperation>& out_diff)
{
  xiiVariant defaultValue = GetDefaultValue(superPtr, pAccessor, pObject, pProp);
  xiiVariant currentValue;
  XII_SUCCEED_OR_RETURN(pAccessor->GetValue(pObject, pProp, currentValue));

  const xiiAbstractObjectGraph* pGraph           = xiiPrefabCache::GetSingleton()->GetCachedPrefabGraph(m_CreateFromPrefab);
  xiiUuid                       objectPrefabGuid = pObject->GetGuid();
  objectPrefabGuid.RevertCombinationWithSeed(m_PrefabSeedGuid);
  if (pGraph)
  {
    // We create a sub-graph of only the parent node in both re-mapped prefab as well as from the actually object. We limit the graph to only the container property.
    auto                   pNode = pGraph->GetNode(objectPrefabGuid);
    xiiAbstractObjectGraph prefabSubGraph;
    pGraph->Clone(prefabSubGraph, pNode, [pRootNode = pNode, pRootProp = pProp](const xiiAbstractObjectNode* pNode, const xiiAbstractObjectNode::Property* pProp) {
      if (pNode == pRootNode && pProp->m_sPropertyName != pRootProp->GetPropertyName())
        return false;

      return true;
    });

    prefabSubGraph.ReMapNodeGuids(m_PrefabSeedGuid);

    xiiAbstractObjectGraph           instanceSubGraph;
    xiiDocumentObjectConverterWriter writer(&instanceSubGraph, pObject->GetDocumentObjectManager(), [pRootObject = pObject, pRootProp = pProp](const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) {
      if (pObject == pRootObject && pProp != pRootProp)
        return false;

      return true;
    });

    writer.AddObjectToGraph(pObject);

    prefabSubGraph.CreateDiffWithBaseGraph(instanceSubGraph, out_diff);

    return XII_SUCCESS;
  }

  return xiiStatus(xiiFmt("The object was not found in the base prefab graph."));
}
