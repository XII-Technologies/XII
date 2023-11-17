#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

using xiiDecalResourceHandle = xiiTypedResourceHandle<class xiiDecalResource>;

struct xiiDecalResourceDescriptor
{
};

class XII_GRAPHICSCORE_DLL xiiDecalResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiDecalResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiDecalResource, xiiDecalResourceDescriptor);

public:
  xiiDecalResource();

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
};

class XII_GRAPHICSCORE_DLL xiiDecalResourceLoader : public xiiResourceTypeLoader
{
public:
  struct LoadedData
  {
    LoadedData() :
      m_Reader(&m_Storage)
    {
    }

    xiiContiguousMemoryStreamStorage m_Storage;
    xiiMemoryStreamReader            m_Reader;
  };

  virtual xiiResourceLoadData OpenDataStream(const xiiResource* pResource) override;
  virtual void                CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& loaderData) override;
  virtual bool                IsResourceOutdated(const xiiResource* pResource) const override;
};
