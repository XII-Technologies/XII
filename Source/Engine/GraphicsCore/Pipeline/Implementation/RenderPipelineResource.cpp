/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineResource, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;
XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiRenderPipelineResource);

xiiRenderPipelineResource::xiiRenderPipelineResource() : xiiResource(DoUpdate::OnAnyThread, 1) {}
xiiSharedPtr<const xiiRenderPipeline> xiiRenderPipelineResource::GetPipeline() const
{
  XII_LOCK(m_Mutex);
  return m_pPipeline;
}
xiiResourceLoadDescription xiiRenderPipelineResource::UnloadData(Unload)
{
  XII_LOCK(m_Mutex);
  m_pPipeline = nullptr;
  return {xiiResourceState::Unloaded, 0, 0};
}
xiiResourceLoadDescription xiiRenderPipelineResource::UpdateContent(xiiStreamReader* stream)
{
  if (!stream)
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  // The standard file loader prefixes every file stream with its absolute source path.
  xiiString path;
  if (stream->ReadString(path).Failed())
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  if (path.HasExtension("xiiBinRenderPipeline"))
  {
    xiiAssetFileHeader header;
    xiiUInt8           version = 0;
    if (header.Read(*stream).Failed() || stream->ReadBytes(&version, sizeof(version)) != sizeof(version))
      return {xiiResourceState::LoadedResourceMissing, 0, 0};
    if (version != 3)
    {
      xiiLog::Error("Render pipeline '{}' uses obsolete compiled format {}. Re-transform its source asset.", path, version);
      return {xiiResourceState::LoadedResourceMissing, 0, 0};
    }
  }
  xiiRenderPipelineResourceDescriptor descriptor;
  if (descriptor.Deserialize(*stream).Failed())
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  return CreateResource(std::move(descriptor));
}
void xiiRenderPipelineResource::UpdateMemoryUsage(MemoryUsage& usage)
{
  XII_LOCK(m_Mutex);
  usage.m_uiMemoryCPU = sizeof(*this);
  usage.m_uiMemoryGPU = 0;
  if (m_pPipeline)
  {
    usage.m_uiMemoryCPU += static_cast<xiiUInt32>(sizeof(xiiRenderPipeline) + m_pPipeline->GetHeapMemoryUsage());
  }
}
XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiRenderPipelineResource, xiiRenderPipelineResourceDescriptor)
{
  xiiSharedPtr<const xiiRenderPipeline> pipeline;
  xiiStringBuilder                      error;
  if (xiiRenderPipeline::Compile(descriptor, pipeline, &error).Failed())
  {
    xiiLog::Error("Render pipeline '{}': {}", GetResourceIdOrDescription(), error);
    return {xiiResourceState::LoadedResourceMissing, 0, 0};
  }
  XII_LOCK(m_Mutex);
  m_pPipeline = std::move(pipeline);
  return {xiiResourceState::Loaded, 0, 0};
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelineResource);
