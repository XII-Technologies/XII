#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/HashTable.h>
#include <RendererCore/RendererCoreDLL.h>

using xiiRenderPipelineResourceHandle = xiiTypedResourceHandle<class xiiRenderPipelineResource>;
class xiiRenderPipeline;

struct xiiRenderPipelineResourceDescriptor
{
  void Clear() {}

  void CreateFromRenderPipeline(const xiiRenderPipeline* pPipeline);

  xiiDynamicArray<xiiUInt8> m_SerializedPipeline;
  xiiString                 m_sPath;
};

class XII_RENDERERCORE_DLL xiiRenderPipelineResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiRenderPipelineResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiRenderPipelineResource, xiiRenderPipelineResourceDescriptor);

public:
  xiiRenderPipelineResource();

  XII_ALWAYS_INLINE const xiiRenderPipelineResourceDescriptor& GetDescriptor() { return m_Desc; }

  xiiInternal::NewInstance<xiiRenderPipeline> CreateRenderPipeline() const;

public:
  static xiiRenderPipelineResourceHandle CreateMissingPipeline();

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiRenderPipelineResourceDescriptor m_Desc;
};
