/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelUploadPass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSparseVoxelUploadPass,1,xiiRTTIDefaultAllocator<xiiSparseVoxelUploadPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Before", m_Before)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("After", m_After)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Schedule",m_Schedule)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PageTable",m_Pages)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LevelData",m_Levels)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("BrickUpdates",m_Updates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Constants",m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiSparseVoxelUploadPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto snapshot = xiiSparseVoxelPassUtils::Snapshot(context, m_Schedule);
  if (!snapshot)
    return context.ReportFailure("Sparse voxel upload requires a valid current scheduling snapshot.");
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiSharedPtr<const xiiSparseVoxelFrameData> m_Snapshot;
    xiiRenderGraphBufferHandle                  m_Pages, m_Levels, m_Updates, m_Constants;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Snapshot=snapshot;
    const auto buffer=[&](xiiStringView name,xiiUInt32 stride,xiiUInt32 count,bool writable){ xiiGALBufferCreationDescription d; d.m_Mode=xiiGALBufferMode::Structured; d.m_Usage=xiiGALResourceUsage::Mutable; d.m_uiSize=xiiUInt64(stride)*xiiMath::Max(count,1U); d.m_uiElementByteStride=stride; d.m_BindFlags=xiiGALBindFlags::ShaderResource; if(writable) d.m_BindFlags|=xiiGALBindFlags::UnorderedAccess; return builder.WriteBuffer(name,d,xiiGALResourceStateFlags::CopyDestination); };
    data.m_Pages=buffer("Page Table",sizeof(xiiUInt32),snapshot->m_Pages.GetCount(),true);
    data.m_Levels=buffer("Clipmap Levels",sizeof(xiiGpuSparseVoxelLevel),snapshot->m_Levels.GetCount(),false);
    data.m_Updates=buffer("Brick Updates",sizeof(xiiGpuSparseVoxelBrickUpdate),snapshot->m_Updates.GetCount(),false);
    data.m_Constants=xiiRenderPassUtils::Constants(builder,"Clipmap Constants",sizeof(xiiSparseVoxelRadianceConstants)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();
    commands.UpdateBuffer(execute.GetBuffer(data.m_Pages),0,xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Snapshot->m_Pages.GetData()),data.m_Snapshot->m_Pages.GetCount()*sizeof(xiiUInt32)));
    commands.UpdateBuffer(execute.GetBuffer(data.m_Levels),0,xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Snapshot->m_Levels.GetData()),data.m_Snapshot->m_Levels.GetCount()*sizeof(xiiGpuSparseVoxelLevel)));
    if(!data.m_Snapshot->m_Updates.IsEmpty()) commands.UpdateBuffer(execute.GetBuffer(data.m_Updates),0,xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Snapshot->m_Updates.GetData()),data.m_Snapshot->m_Updates.GetCount()*sizeof(xiiGpuSparseVoxelBrickUpdate)));
    { xiiGALMapHelper<xiiSparseVoxelRadianceConstants> mapped(commands,execute.GetBuffer(data.m_Constants),xiiGALMapType::Write,xiiGALMapFlags::Discard); *mapped=data.m_Snapshot->m_Constants; }
    xiiGALStateTransitionDescription transition; transition.m_pResource=execute.GetBuffer(data.m_Constants); transition.m_NewState=xiiGALResourceStateFlags::ConstantBuffer; transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState; commands.TransitionResourceStates(xiiMakeArrayPtr(&transition,1)); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Pages, pass.first->m_Pages));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Levels, pass.first->m_Levels));
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Updates, pass.first->m_Updates));
  return context.SetOutput(m_Constants, pass.first->m_Constants);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_SparseVoxelUploadPass);
