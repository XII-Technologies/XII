#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Serialization/RttiConverter.h>

class xiiRenderPipeline;
struct xiiRenderPipelineResourceDescriptor;
class xiiStreamWriter;
class xiiRenderPipelinePassBase;
class xiiExtractor;

struct XII_GRAPHICSCORE_DLL xiiRenderPipelineResourceLoaderConnection
{
  xiiUInt32 m_uiSource;
  xiiUInt32 m_uiTarget;
  xiiString m_sSourcePin;
  xiiString m_sTargetPin;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderPipelineResourceLoaderConnection);

struct XII_GRAPHICSCORE_DLL xiiRenderPipelineResourceLoader
{
  static xiiInternal::NewInstance<xiiRenderPipeline> CreateRenderPipeline(const xiiRenderPipelineResourceDescriptor& desc);
  static void                                        CreateRenderPipelineResourceDescriptor(const xiiRenderPipeline* pPipeline, xiiRenderPipelineResourceDescriptor& ref_desc);
  static xiiResult                                   ExportPipeline(xiiArrayPtr<const xiiRenderPipelinePassBase* const> pPasses, xiiArrayPtr<const xiiExtractor* const> pExtractors, xiiArrayPtr<const xiiRenderPipelineResourceLoaderConnection> pConnections, xiiStreamWriter& ref_streamWriter);
};
