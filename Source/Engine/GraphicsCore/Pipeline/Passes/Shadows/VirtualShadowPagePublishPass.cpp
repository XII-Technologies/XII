/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowPagePublishPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVirtualShadowPagePublishPass, 1, xiiRTTIDefaultAllocator<xiiVirtualShadowPagePublishPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RasterPages", m_Pages)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("RasterAtlas", m_Atlas)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PhysicalPageTable", m_Physical)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VirtualPageTable", m_Virtual)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ReadyAtlas", m_ReadyAtlas)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ReadyPhysicalPageTable", m_ReadyPhysical)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ReadyVirtualPageTable", m_ReadyVirtual)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Shadows") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiVirtualShadowPagePublishPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiUInt64 completed = 0;
  if (xiiRenderGraphManager::GetFrameCompletion(context.m_Graph.GetFrameIndex(), completed).Failed())
    return context.ReportFailure("Page publication requires an active shared GPU frame.");
  const auto batch = context.GetInputData(m_Pages).Downcast<const xiiVirtualShadowRasterBatch>();
  if (!batch || !batch->m_Residency || batch->m_Residency->GetSetupSerial() != context.m_Graph.GetSetupSerial())
    return context.ReportFailure("Page publication requires an immutable batch from this graph setup.");
  auto        atlas    = context.GetInputTexture(m_Atlas);
  auto        physical = context.GetInputBuffer(m_Physical), table = context.GetInputBuffer(m_Virtual);
  const auto& source = batch->m_Residency->m_Bindings;
  if (atlas.GetResourceId() != source.m_hPhysicalAtlas.GetResourceId() || physical.GetResourceId() != source.m_hPhysicalPageTable.GetResourceId() || table.GetResourceId() != source.m_hVirtualPageTable.GetResourceId())
    return context.ReportFailure("Page publication inputs do not belong to the batch's captured device allocations.");
  const auto publication = xiiVirtualShadowMapManager::CapturePagePublication(batch->m_Residency->m_Upload, batch->m_Pages);
  if (!publication)
    return context.ReportFailure("Page publication contains stale, duplicate or unmapped raster tickets.");
  if (!publication->m_Records.IsEmpty())
  {
    const auto completion = xiiGALDevice::GetDefaultDevice()->CreateFence({});
    if (!completion)
      return context.ReportFailure("Could not allocate page publication completion fence.");
    struct Data
    {
      xiiRenderGraphTextureHandle                         m_Atlas;
      xiiRenderGraphBufferHandle                          m_Physical, m_Virtual;
      xiiSharedPtr<const xiiVirtualShadowPagePublication> m_Publication;
      xiiSharedPtr<xiiGALFence>                           m_Completion;
    };
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Atlas = builder.ReadTexture(atlas, xiiGALResourceStateFlags::ShaderResource);
      // These are sparse edits: unmodified records and parent upload dependencies
      // must survive, including readers of earlier versions of the same allocation.
      data.m_Physical = builder.ReadWriteBuffer(physical, xiiGALResourceStateFlags::CopyDestination);
      data.m_Virtual = builder.ReadWriteBuffer(table, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_Physical, xiiGALResourceStateFlags::ShaderResource);
      builder.ExportBuffer(data.m_Virtual, xiiGALResourceStateFlags::ShaderResource);
      data.m_Publication = publication; data.m_Completion = completion; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      for (const auto& record : data.m_Publication->m_Records)
      {
        if (!xiiVirtualShadowMapManager::IsRasterWorkCurrent(record.m_Ticket)) continue;
        const auto bytes = xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&record.m_CleanRecord), sizeof(record.m_CleanRecord));
        execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Physical), record.m_uiPhysicalByteOffset, bytes);
        execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Virtual), record.m_uiVirtualByteOffset, bytes);
      }
      execute.GetCommandList().EnqueueSignal(data.m_Completion.Borrow(), 1);
      xiiVirtualShadowMapManager::RecordPagePublication(*data.m_Publication, data.m_Completion); }, true);
    atlas              = emitted.first->m_Atlas;
    physical           = emitted.first->m_Physical;
    table              = emitted.first->m_Virtual;
  }
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_ReadyAtlas, atlas));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_ReadyPhysical, physical));
  return context.SetOutput(m_ReadyVirtual, table);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Shadows_VirtualShadowPagePublishPass);
