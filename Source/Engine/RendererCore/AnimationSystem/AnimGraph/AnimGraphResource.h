#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>

class xiiAnimGraph;

//////////////////////////////////////////////////////////////////////////

using xiiAnimGraphResourceHandle = xiiTypedResourceHandle<class xiiAnimGraphResource>;

class XII_RENDERERCORE_DLL xiiAnimGraphResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiAnimGraphResource);

public:
  xiiAnimGraphResource();
  ~xiiAnimGraphResource();

  void DeserializeAnimGraphState(xiiAnimGraph& out);

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiDataBuffer m_Storage;
};
