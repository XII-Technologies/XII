/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelPoolPreparePass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/GlobalIllumination/SparseVoxelPoolConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSparseVoxelPoolPreparePass,1,xiiRTTIDefaultAllocator<xiiSparseVoxelPoolPreparePass>)
{
  XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Schedule",m_Schedule)->AddFlags(xiiPropertyFlags::ReadOnly), XII_MEMBER_PROPERTY("PreviousPool",m_Previous)->AddFlags(xiiPropertyFlags::ReadOnly), XII_MEMBER_PROPERTY("RadiancePool",m_Pool)->AddFlags(xiiPropertyFlags::ReadOnly), } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiSparseVoxelPoolPreparePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto snapshot = xiiSparseVoxelPassUtils::Snapshot(context, m_Schedule);
  if (!snapshot)
    return context.ReportFailure("Sparse pool preparation requires a current scheduling snapshot.");
  const auto previous = context.GetInputBuffer(m_Previous);
  if (snapshot->m_bHistoryValid && !xiiLightClusterPassUtils::Buffer(context, previous, sizeof(xiiVec4), snapshot->m_PoolCount, xiiGALBindFlags::ShaderResource))
    return context.ReportFailure("Sparse pool preparation must receive the same history allocation as scheduling.");
  xiiLightClusterConstants         dispatchData{};
  xiiGALDispatchComputeDescription dispatch;
  if (xiiLightClusterPassUtils::Dispatch(dispatchData, snapshot->m_PoolCount, dispatch).Failed())
    return context.ReportFailure("Sparse pool capacity exceeds device dispatch limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/GlobalIllumination/SparseVoxelPoolPrepare.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle               m_Previous, m_Pool, m_Constants;
    xiiSparseVoxelPoolConstants              m_Snapshot;
    xiiGALDispatchComputeDescription         m_Dispatch;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Pipeline=m_Pipeline; data.m_Dispatch=dispatch; data.m_Snapshot.PoolWork=xiiVec4U32(snapshot->m_PoolCount,dispatchData.Work.z,snapshot->m_bHistoryValid ? 1U:0U,0);
    auto description=xiiLightClusterPassUtils::Description(sizeof(xiiVec4),snapshot->m_PoolCount); data.m_Pool=builder.WriteBuffer("Current Radiance Pool",description,xiiGALResourceStateFlags::UnorderedAccess);
    if(snapshot->m_bHistoryValid) data.m_Previous=builder.ReadBuffer(previous,xiiGALResourceStateFlags::ShaderResource);
    else { auto empty=xiiLightClusterPassUtils::Description(sizeof(xiiVec4),1); empty.m_Usage=xiiGALResourceUsage::Mutable; empty.m_BindFlags=xiiGALBindFlags::ShaderResource; data.m_Previous=builder.WriteBuffer("Unused Previous Pool",empty,xiiGALResourceStateFlags::CopyDestination); }
    data.m_Constants=xiiRenderPassUtils::Constants(builder,"Pool Preparation",sizeof(xiiSparseVoxelPoolConstants)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();
    if(!data.m_Snapshot.PoolWork.z)
    {
      const auto zero=xiiVec4::MakeZero(); commands.UpdateBuffer(execute.GetBuffer(data.m_Previous),0,xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&zero),sizeof(zero)));
      xiiGALStateTransitionDescription previous; previous.m_pResource=execute.GetBuffer(data.m_Previous); previous.m_NewState=xiiGALResourceStateFlags::ShaderResource; previous.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState; commands.TransitionResourceStates(xiiMakeArrayPtr(&previous,1));
    }
    { xiiGALMapHelper<xiiSparseVoxelPoolConstants> mapped(commands,execute.GetBuffer(data.m_Constants),xiiGALMapType::Write,xiiGALMapFlags::Discard); *mapped=data.m_Snapshot; }
    xiiGALStateTransitionDescription transition; transition.m_pResource=execute.GetBuffer(data.m_Constants); transition.m_NewState=xiiGALResourceStateFlags::ConstantBuffer; transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState; commands.TransitionResourceStates(xiiMakeArrayPtr(&transition,1));
    commands.SetPipelineState(data.m_Pipeline.Borrow()); commands.ResolveAndSetConstantBuffer("xiiSparseVoxelPoolConstants",execute.GetBuffer(data.m_Constants),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_PreviousPool",execute.GetBuffer(data.m_Previous)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_CurrentPool",execute.GetBuffer(data.m_Pool)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess(); commands.DispatchCompute(data.m_Dispatch); });
  return context.SetOutput(m_Pool, pass.first->m_Pool);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_SparseVoxelPoolPreparePass);
