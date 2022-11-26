#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/PropertyPath.h>

using xiiPrefabResourceHandle = xiiTypedResourceHandle<class xiiPrefabResource>;

struct XII_CORE_DLL xiiPrefabResourceDescriptor
{
};

struct XII_CORE_DLL xiiExposedPrefabParameterDesc
{
  xiiHashedString m_sExposeName;
  xiiUInt32       m_uiWorldReaderChildObject : 1; // 0 -> use root object array, 1 -> use child object array
  xiiUInt32       m_uiWorldReaderObjectIndex : 31;
  xiiHashedString m_sComponentType;     // xiiRTTI type name to identify which component is meant, empty string -> affects game object
  xiiHashedString m_sProperty;          // which property to override
  xiiPropertyPath m_CachedPropertyPath; // cached xiiPropertyPath to apply a value to the specified property

  void Save(xiiStreamWriter& stream) const;
  void Load(xiiStreamReader& stream);
  void LoadOld(xiiStreamReader& stream);
};

class XII_CORE_DLL xiiPrefabResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPrefabResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiPrefabResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiPrefabResource, xiiPrefabResourceDescriptor);

public:
  xiiPrefabResource();

  /// \brief Creates an instance of this prefab in the given world.
  void InstantiatePrefab(xiiWorld& world, const xiiTransform& rootTransform, xiiPrefabInstantiationOptions options, const xiiArrayMap<xiiHashedString, xiiVariant>* pExposedParamValues = nullptr);

  void ApplyExposedParameterValues(const xiiArrayMap<xiiHashedString, xiiVariant>* pExposedParamValues, const xiiDynamicArray<xiiGameObject*>& createdChildObjects, const xiiDynamicArray<xiiGameObject*>& createdRootObjects) const;

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiUInt32 FindFirstParamWithName(xiiUInt64 uiNameHash) const;

  xiiWorldReader                                 m_WorldReader;
  xiiDynamicArray<xiiExposedPrefabParameterDesc> m_PrefabParamDescs;
};
