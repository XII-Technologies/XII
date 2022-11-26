#pragma once

#include <Foundation/Serialization/RttiConverter.h>
#include <RendererCore/RendererCoreDLL.h>

class xiiRenderPipeline;
struct xiiRenderPipelineResourceDescriptor;

struct XII_RENDERERCORE_DLL xiiRenderPipelineResourceLoader
{
  static xiiInternal::NewInstance<xiiRenderPipeline> CreateRenderPipeline(const xiiRenderPipelineResourceDescriptor& desc);
  static void                                        CreateRenderPipelineResourceDescriptor(const xiiRenderPipeline* pPipeline, xiiRenderPipelineResourceDescriptor& desc);
};

class XII_RENDERERCORE_DLL xiiRenderPipelineRttiConverterContext : public xiiRttiConverterContext
{
public:
  xiiRenderPipelineRttiConverterContext() :
    m_pRenderPipeline(nullptr)
  {
  }

  virtual void Clear() override;

  virtual xiiInternal::NewInstance<void> CreateObject(const xiiUuid& guid, const xiiRTTI* pRtti) override;
  virtual void                           DeleteObject(const xiiUuid& guid) override;

  xiiRenderPipeline* m_pRenderPipeline;
};
