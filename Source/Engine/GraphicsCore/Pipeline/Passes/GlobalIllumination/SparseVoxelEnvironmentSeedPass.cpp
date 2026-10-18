/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/Containers/HashSet.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelEnvironmentSeedPass.h>
#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/SparseVoxelPassUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/GlobalIllumination/SparseVoxelPoolConstants.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSparseVoxelEnvironmentSeedPass,1,xiiRTTIDefaultAllocator<xiiSparseVoxelEnvironmentSeedPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Schedule",m_Schedule)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("EnvironmentLight",m_Environment)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Constants",m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("LevelData",m_Levels)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("BrickUpdates",m_Updates)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitialPageTable",m_InitialPages)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InitialPool",m_InitialPool)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PageTable",m_Pages)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("RadiancePool",m_Pool)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GlobalIllumination") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiSparseVoxelEnvironmentSeedPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto snapshot = xiiSparseVoxelPassUtils::Snapshot(context, m_Schedule);
  if (!snapshot)
    return context.ReportFailure("Sparse environment seed requires a valid current schedule.");
  const auto  constants = context.GetInputBuffer(m_Constants), environment = context.GetInputBuffer(m_Environment), levels = context.GetInputBuffer(m_Levels), updates = context.GetInputBuffer(m_Updates), pages = context.GetInputBuffer(m_InitialPages), pool = context.GetInputBuffer(m_InitialPool);
  const auto* c = context.m_Graph.GetBufferDescription(constants);
  const auto* e = context.m_Graph.GetBufferDescription(environment);
  using xiiLightClusterPassUtils::Buffer;
  const auto writable = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  if (!c || c->m_uiSize != sizeof(xiiSparseVoxelRadianceConstants) || !e || e->m_uiSize != sizeof(xiiEnvironmentLightConstants) || !Buffer(context, levels, sizeof(xiiGpuSparseVoxelLevel), snapshot->m_Levels.GetCount(), xiiGALBindFlags::ShaderResource) ||
      !Buffer(context, updates, sizeof(xiiGpuSparseVoxelBrickUpdate), xiiMath::Max(snapshot->m_Updates.GetCount(), 1U), xiiGALBindFlags::ShaderResource) || !Buffer(context, pages, sizeof(xiiUInt32), snapshot->m_Pages.GetCount(), writable) || !Buffer(context, pool, sizeof(xiiVec4), snapshot->m_PoolCount, writable) || pages.m_Id == pool.m_Id)
    return context.ReportFailure("Sparse seed resources must exactly match the schedule and have distinct writable allocations.");
  xiiHashSet<xiiUInt32> seen;
  for (const auto& update : snapshot->m_Updates)
  {
    if (update.m_vPhysicalLevelAndKey.x >= snapshot->m_Constants.PoolLayout.y || seen.Contains(update.m_vPhysicalLevelAndKey.x))
      return context.ReportFailure("Sparse update slots must be unique and within capacity.");
    seen.Insert(update.m_vPhysicalLevelAndKey.x);
  }
  const auto                       work = snapshot->m_Updates.GetCount() * snapshot->m_Constants.PoolLayout.x;
  xiiLightClusterConstants         dimensions{};
  xiiGALDispatchComputeDescription dispatch;
  if (xiiLightClusterPassUtils::Dispatch(dimensions, xiiMath::Max(work, 1U), dispatch).Failed())
    return context.ReportFailure("Sparse update budget exceeds dispatch limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/GlobalIllumination/SparseVoxelEnvironmentSeed.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle                  m_Constants, m_Environment, m_Levels, m_Updates, m_Pages, m_Pool, m_Work;
    xiiUInt32                                   m_Width;
    xiiGALDispatchComputeDescription            m_Dispatch;
    xiiSharedPtr<xiiGALComputePipelineState>    m_Pipeline;
    xiiSharedPtr<const xiiSparseVoxelFrameData> m_Snapshot;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Snapshot=snapshot; data.m_Pipeline=m_Pipeline; data.m_Dispatch=dispatch; data.m_Width=dimensions.Work.z;
    data.m_Constants=builder.ReadBuffer(constants,xiiGALResourceStateFlags::ConstantBuffer); data.m_Environment=builder.ReadBuffer(environment,xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Levels=builder.ReadBuffer(levels,xiiGALResourceStateFlags::ShaderResource); data.m_Updates=builder.ReadBuffer(updates,xiiGALResourceStateFlags::ShaderResource);
    data.m_Pages=builder.ReadWriteBuffer(pages,xiiGALResourceStateFlags::UnorderedAccess); data.m_Pool=builder.ReadWriteBuffer(pool,xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Work=xiiRenderPassUtils::Constants(builder,"Sparse Dispatch",sizeof(xiiSparseVoxelPoolConstants)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();
    { xiiGALMapHelper<xiiSparseVoxelPoolConstants> mapped(commands,execute.GetBuffer(data.m_Work),xiiGALMapType::Write,xiiGALMapFlags::Discard); mapped->PoolWork=xiiVec4U32(0,data.m_Width,0,0); }
    xiiGALStateTransitionDescription transition; transition.m_pResource=execute.GetBuffer(data.m_Work); transition.m_NewState=xiiGALResourceStateFlags::ConstantBuffer; transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState; commands.TransitionResourceStates(xiiMakeArrayPtr(&transition,1));
    commands.SetPipelineState(data.m_Pipeline.Borrow());
    commands.ResolveAndSetConstantBuffer("xiiSparseVoxelPoolConstants",execute.GetBuffer(data.m_Work),xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiSparseVoxelRadianceConstants",execute.GetBuffer(data.m_Constants),xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiEnvironmentLightConstants",execute.GetBuffer(data.m_Environment),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_BrickUpdates",execute.GetBuffer(data.m_Updates)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_Levels",execute.GetBuffer(data.m_Levels)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_PageTable",execute.GetBuffer(data.m_Pages)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_RadiancePool",execute.GetBuffer(data.m_Pool)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess(); commands.DispatchCompute(data.m_Dispatch); data.m_Snapshot->CommitUpdates(); });
  XII_SUCCEED_OR_RETURN(context.SetOutput(m_Pages, pass.first->m_Pages));
  return context.SetOutput(m_Pool, pass.first->m_Pool);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_SparseVoxelEnvironmentSeedPass);
