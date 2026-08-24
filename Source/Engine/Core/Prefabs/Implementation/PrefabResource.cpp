/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPrefabResource, 1, xiiRTTIDefaultAllocator<xiiPrefabResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiPrefabResource);
// clang-format on

xiiPrefabResource::xiiPrefabResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

void xiiPrefabResource::InstantiatePrefab(xiiWorld& ref_world, const xiiTransform& rootTransform, xiiPrefabInstantiationOptions options, const xiiArrayMap<xiiHashedString, xiiVariant>* pExposedParamValues)
{
  if (GetLoadingState() != xiiResourceState::Loaded)
    return;

  if (pExposedParamValues != nullptr && !pExposedParamValues->IsEmpty())
  {
    xiiTemporaryHybridArray<xiiGameObject*, 8> createdRootObjects;
    xiiTemporaryHybridArray<xiiGameObject*, 8> createdChildObjects;

    if (options.m_pCreatedRootObjectsOut == nullptr)
    {
      options.m_pCreatedRootObjectsOut = &createdRootObjects;
    }

    if (options.m_pCreatedChildObjectsOut == nullptr)
    {
      options.m_pCreatedChildObjectsOut = &createdChildObjects;
    }

    m_WorldReader.InstantiatePrefab(ref_world, rootTransform, options);

    XII_ASSERT_DEBUG(options.m_pCreatedRootObjectsOut != options.m_pCreatedChildObjectsOut, "These pointers must point to different arrays, otherwise applying exposed properties doesn't work correctly.");
    ApplyExposedParameterValues(pExposedParamValues, *options.m_pCreatedChildObjectsOut, *options.m_pCreatedRootObjectsOut);
  }
  else
  {
    m_WorldReader.InstantiatePrefab(ref_world, rootTransform, options);
  }
}

xiiPrefabResource::InstantiateResult xiiPrefabResource::InstantiatePrefab(const xiiPrefabResourceHandle& hPrefab, bool bBlockTillLoaded, xiiWorld& ref_world, const xiiTransform& rootTransform, xiiPrefabInstantiationOptions options, const xiiArrayMap<xiiHashedString, xiiVariant>* pExposedParamValues /*= nullptr*/)
{
  xiiResourceLock<xiiPrefabResource> pPrefab(hPrefab, bBlockTillLoaded ? xiiResourceAcquireMode::BlockTillLoaded_NeverFail : xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);

  switch (pPrefab.GetAcquireResult())
  {
    case xiiResourceAcquireResult::Final:
      pPrefab->InstantiatePrefab(ref_world, rootTransform, options, pExposedParamValues);
      return InstantiateResult::Success;

    case xiiResourceAcquireResult::LoadingFallback:
      return InstantiateResult::NotYetLoaded;

    default:
      return InstantiateResult::Error;
  }
}

void xiiPrefabResource::ApplyExposedParameterValues(const xiiArrayMap<xiiHashedString, xiiVariant>* pExposedParamValues, const xiiDynamicArray<xiiGameObject*>& createdChildObjects, const xiiDynamicArray<xiiGameObject*>& createdRootObjects) const
{
  const xiiUInt32 uiNumParamDescs = m_PrefabParamDescs.GetCount();

  for (xiiUInt32 i = 0; i < pExposedParamValues->GetCount(); ++i)
  {
    const xiiHashedString& name       = pExposedParamValues->GetKey(i);
    const xiiUInt64        uiNameHash = name.GetHash();

    for (xiiUInt32 uiCurParam = FindFirstParamWithName(uiNameHash); uiCurParam < uiNumParamDescs; ++uiCurParam)
    {
      const auto& ppd = m_PrefabParamDescs[uiCurParam];

      if (ppd.m_sExposeName.GetHash() != uiNameHash)
        break;

      xiiGameObject* pTarget = ppd.m_uiWorldReaderChildObject ? createdChildObjects[ppd.m_uiWorldReaderObjectIndex] : createdRootObjects[ppd.m_uiWorldReaderObjectIndex];

      if (ppd.m_CachedPropertyPath.IsValid())
      {
        if (ppd.m_sComponentType.IsEmpty())
        {
          ppd.m_CachedPropertyPath.SetValue(pTarget, pExposedParamValues->GetValue(i));
        }
        else
        {
          for (xiiComponent* pComp : pTarget->GetComponents())
          {
            const xiiRTTI* pRtti = pComp->GetDynamicRTTI();

            // TODO: use component index instead
            // atm if the same component type is attached multiple times, they will all get the value applied
            if (pRtti->GetTypeNameHash() == ppd.m_sComponentType.GetHash())
            {
              ppd.m_CachedPropertyPath.SetValue(pComp, pExposedParamValues->GetValue(i));
            }
          }
        }
      }

      // Allow to bind multiple properties to the same exposed parameter name
      // Therefore, do not break here, but continue iterating
    }
  }
}

xiiResourceLoadDescription xiiPrefabResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  if (WhatToUnload == xiiResource::Unload::AllQualityLevels)
  {
    m_WorldReader.ClearAndCompact();
  }

  return res;
}

xiiResourceLoadDescription xiiPrefabResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_LOG_BLOCK("xiiPrefabResource::UpdateContent", GetResourceIdOrDescription());

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  xiiStreamReader& s = *Stream;

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiString sAbsFilePath;
    s >> sAbsFilePath;
  }

  xiiAssetFileHeader assetHeader;
  assetHeader.Read(s).IgnoreResult();

  char szSceneTag[16];
  s.ReadBytes(szSceneTag, sizeof(char) * 16);
  XII_ASSERT_DEV(xiiStringUtils::IsEqualN(szSceneTag, "[xiiBinaryScene]", 16), "The given file is not a valid prefab file");

  if (!xiiStringUtils::IsEqualN(szSceneTag, "[xiiBinaryScene]", 16))
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  m_WorldReader.ReadWorldDescription(s).IgnoreResult();

  if (assetHeader.GetFileVersion() >= 4)
  {
    xiiUInt32 uiExposedParams = 0;

    s >> uiExposedParams;

    m_PrefabParamDescs.SetCount(uiExposedParams);

    for (xiiUInt32 i = 0; i < uiExposedParams; ++i)
    {
      auto& ppd = m_PrefabParamDescs[i];

      XII_ASSERT_DEV(assetHeader.GetFileVersion() >= 6, "Old resource version not supported anymore");
      ppd.Load(s);

      // initialize the cached property path here once
      // so we can only apply it later as often as needed
      {
        if (ppd.m_sComponentType.IsEmpty())
        {
          ppd.m_CachedPropertyPath.InitializeFromPath(*xiiGetStaticRTTI<xiiGameObject>(), ppd.m_sProperty).IgnoreResult();
        }
        else
        {
          if (const xiiRTTI* pRtti = xiiRTTI::FindTypeByNameHash(ppd.m_sComponentType.GetHash()))
          {
            ppd.m_CachedPropertyPath.InitializeFromPath(*pRtti, ppd.m_sProperty).IgnoreResult();
          }
        }
      }
    }

    // sort exposed parameter descriptions by name hash for quicker access
    m_PrefabParamDescs.Sort([](const xiiExposedPrefabParameterDesc& lhs, const xiiExposedPrefabParameterDesc& rhs) -> bool { return lhs.m_sExposeName.GetHash() < rhs.m_sExposeName.GetHash(); });
  }

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiPrefabResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = m_WorldReader.GetHeapMemoryUsage() + sizeof(this);
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiPrefabResource, xiiPrefabResourceDescriptor)
{
  XII_IGNORE_UNUSED(descriptor);

  xiiResourceLoadDescription desc;
  desc.m_State                      = xiiResourceState::Loaded;
  desc.m_uiQualityLevelsDiscardable = 0;
  desc.m_uiQualityLevelsLoadable    = 0;
  return desc;
}

xiiUInt32 xiiPrefabResource::FindFirstParamWithName(xiiUInt64 uiNameHash) const
{
  xiiUInt32 lb = 0;
  xiiUInt32 ub = m_PrefabParamDescs.GetCount();

  while (lb < ub)
  {
    const xiiUInt32 middle = lb + ((ub - lb) >> 1);

    if (m_PrefabParamDescs[middle].m_sExposeName.GetHash() < uiNameHash)
    {
      lb = middle + 1;
    }
    else
    {
      ub = middle;
    }
  }

  return lb;
}

void xiiExposedPrefabParameterDesc::Save(xiiStreamWriter& ref_stream) const
{
  xiiUInt32 comb = m_uiWorldReaderObjectIndex | (m_uiWorldReaderChildObject << 31);

  ref_stream << m_sExposeName;
  ref_stream << comb;
  ref_stream << m_sComponentType;
  ref_stream << m_sProperty;
}

void xiiExposedPrefabParameterDesc::Load(xiiStreamReader& ref_stream)
{
  xiiUInt32 comb = 0;

  ref_stream >> m_sExposeName;
  ref_stream >> comb;
  ref_stream >> m_sComponentType;
  ref_stream >> m_sProperty;

  m_uiWorldReaderObjectIndex = comb & 0x7FFFFFFF;
  m_uiWorldReaderChildObject = (comb >> 31);
}

XII_STATICLINK_FILE(Core, Core_Prefabs_Implementation_PrefabResource);
