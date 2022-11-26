#pragma once

#include <Core/ResourceManager/Resource.h>
#include <ProcGenPlugin/Declarations.h>

using xiiProcGenGraphResourceHandle = xiiTypedResourceHandle<class xiiProcGenGraphResource>;

struct XII_PROCGENPLUGIN_DLL xiiProcGenGraphResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class XII_PROCGENPLUGIN_DLL xiiProcGenGraphResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGenGraphResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiProcGenGraphResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiProcGenGraphResource, xiiProcGenGraphResourceDescriptor);

public:
  xiiProcGenGraphResource();
  ~xiiProcGenGraphResource();

  const xiiDynamicArray<xiiSharedPtr<const xiiProcGenInternal::PlacementOutput>>&   GetPlacementOutputs() const;
  const xiiDynamicArray<xiiSharedPtr<const xiiProcGenInternal::VertexColorOutput>>& GetVertexColorOutputs() const;

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiDynamicArray<xiiSharedPtr<const xiiProcGenInternal::PlacementOutput>>   m_PlacementOutputs;
  xiiDynamicArray<xiiSharedPtr<const xiiProcGenInternal::VertexColorOutput>> m_VertexColorOutputs;

  xiiSharedPtr<xiiProcGenInternal::GraphSharedDataBase> m_pSharedData;
};
