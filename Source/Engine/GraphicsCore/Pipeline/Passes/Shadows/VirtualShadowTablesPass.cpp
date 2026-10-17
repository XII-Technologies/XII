/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/VirtualShadowTablesPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVirtualShadowTablesPass, 1, xiiRTTIDefaultAllocator<xiiVirtualShadowTablesPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PhysicalPageTable", m_Physical)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Residency", m_Residency)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("VirtualPageTable", m_Virtual)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PhysicalAtlas", m_Atlas)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Shadows") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiVirtualShadowTablesPass::Build(xiiRenderPipelinePassContext& context)
{
  m_uiUploadBytes = m_uiUploadRanges = 0;
  xiiVirtualShadowTableBindings snapshot;
  xiiUInt64                     completed = 0;
  if (xiiRenderGraphManager::GetFrameCompletion(context.m_Graph.GetFrameIndex(), completed).Failed())
    return context.ReportFailure("Virtual shadow table uploads require an active shared GPU frame.");
  const auto capture = xiiVirtualShadowMapManager::CaptureTableUpload(context.m_Graph.GetFrameIndex());
  if (!capture)
    return context.ReportFailure("Could not capture configured virtual shadow device residency.");
  // Physical allocations are shared at device scope. Canonical setup names only
  // deduplicate their upload; all execution and consumer bindings use typed handles.
  const xiiString  previous(context.m_Graph.GetResourceNamespace());
  xiiStringBuilder canonical;
  canonical.SetFormat("Device/VirtualShadowTables/{}/{}", capture->m_uiConfigurationRevision, capture->m_uiResidencyRevision);
  context.m_Graph.SetResourceNamespace(canonical);
  XII_SCOPE_EXIT(context.m_Graph.SetResourceNamespace(previous));
  snapshot.m_hPhysicalPageTable = context.m_Graph.FindBuffer("Physical");
  snapshot.m_hVirtualPageTable  = context.m_Graph.FindBuffer("Virtual");
  snapshot.m_hPhysicalAtlas     = context.m_Graph.FindTexture("Atlas");
  if (!snapshot.m_hPhysicalPageTable.IsValid() || !snapshot.m_hVirtualPageTable.IsValid() || !snapshot.m_hPhysicalAtlas.IsValid())
  {
    struct Data
    {
      xiiVirtualShadowTableBindings                   m_Bindings;
      xiiSharedPtr<const xiiVirtualShadowTableUpload> m_Capture;
      xiiSharedPtr<xiiGALFence>                       m_Completion;
    };
    const bool changes    = !xiiVirtualShadowMapManager::IsTableUploadRecorded(*capture) && (!capture->m_PhysicalRecords.IsEmpty() || !capture->m_VirtualRecords.IsEmpty());
    auto       completion = changes ? xiiGALDevice::GetDefaultDevice()->CreateFence({}) : xiiSharedPtr<xiiGALFence>();
    if (changes && !completion)
      return context.ReportFailure("Could not allocate virtual shadow upload completion fence.");
    const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Capture = capture;
      data.m_Completion = completion;
      data.m_Bindings.m_hPhysicalPageTable = builder.ImportBuffer("Physical", capture->m_PhysicalBuffer, capture->m_PhysicalBuffer->GetResourceState());
      data.m_Bindings.m_hVirtualPageTable = builder.ImportBuffer("Virtual", capture->m_VirtualBuffer, capture->m_VirtualBuffer->GetResourceState());
      data.m_Bindings.m_hPhysicalAtlas = builder.ImportTexture("Atlas", capture->m_Atlas, capture->m_Atlas->GetResourceState());
      // A preserved graphics version establishes cross-graph readiness for every
      // reader queue. Earlier uploads use this same queue; this frozen frame's
      // tables are never rewritten underneath an earlier view's compute readers.
      data.m_Bindings.m_hPhysicalPageTable = changes && !capture->m_PhysicalRecords.IsEmpty() ?
        builder.WriteBuffer(data.m_Bindings.m_hPhysicalPageTable, xiiGALResourceStateFlags::CopyDestination) :
        builder.ReadWriteBuffer(data.m_Bindings.m_hPhysicalPageTable, xiiGALResourceStateFlags::ShaderResource);
      data.m_Bindings.m_hVirtualPageTable = changes && !capture->m_VirtualRecords.IsEmpty() ?
        builder.WriteBuffer(data.m_Bindings.m_hVirtualPageTable, xiiGALResourceStateFlags::CopyDestination) :
        builder.ReadWriteBuffer(data.m_Bindings.m_hVirtualPageTable, xiiGALResourceStateFlags::ShaderResource);
      if (changes)
      {
        builder.ExportBuffer(data.m_Bindings.m_hPhysicalPageTable, xiiGALResourceStateFlags::ShaderResource);
        builder.ExportBuffer(data.m_Bindings.m_hVirtualPageTable, xiiGALResourceStateFlags::ShaderResource);
      }
      builder.SetPassAllowMerge(false); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      const auto& captured = *data.m_Capture;
      if (!data.m_Completion || xiiVirtualShadowMapManager::IsTableUploadRecorded(captured)) return;
      for (const auto& range : captured.m_PhysicalRanges)
        execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Bindings.m_hPhysicalPageTable),
          (captured.m_Layout.m_uiFrameBaseIndex + range.m_uiFirstPhysicalPage) * sizeof(xiiGpuVirtualShadowPage),
          xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(captured.m_PhysicalRecords.GetData() + range.m_uiFirstRecord), range.m_uiRecordCount * sizeof(xiiGpuVirtualShadowPage)));
      if (!captured.m_VirtualRecords.IsEmpty())
        execute.GetCommandList().UpdateBuffer(execute.GetBuffer(data.m_Bindings.m_hVirtualPageTable), captured.m_Layout.m_uiVirtualTableBaseIndex * sizeof(xiiGpuVirtualShadowPage),
          xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(captured.m_VirtualRecords.GetData()), captured.m_VirtualRecords.GetCount() * sizeof(xiiGpuVirtualShadowPage)));
      execute.GetCommandList().EnqueueSignal(data.m_Completion.Borrow(), 1);
      xiiVirtualShadowMapManager::RecordTableUpload(data.m_Capture, data.m_Completion); }, changes);
    snapshot           = emitted.first->m_Bindings;
    m_uiUploadBytes    = changes ? xiiUInt64(capture->m_PhysicalRecords.GetCount() + capture->m_VirtualRecords.GetCount()) * sizeof(xiiGpuVirtualShadowPage) : 0;
    m_uiUploadRanges   = changes ? capture->m_PhysicalRanges.GetCount() + (capture->m_VirtualRecords.IsEmpty() ? 0U : 1U) : 0;
  }
  static_cast<xiiVirtualShadowGpuLayout&>(snapshot) = capture->m_Layout;
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Physical, snapshot.m_hPhysicalPageTable));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Virtual, snapshot.m_hVirtualPageTable));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Atlas, snapshot.m_hPhysicalAtlas));
  return context.SetOutput(m_Residency, XII_DEFAULT_NEW(xiiVirtualShadowResidencySnapshot, context.m_Graph.GetSetupSerial(), snapshot, capture));
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Shadows_VirtualShadowTablesPass);
