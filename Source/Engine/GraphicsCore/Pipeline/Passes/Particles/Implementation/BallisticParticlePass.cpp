/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Particles/ParticleSystem.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Particles/BallisticParticlePass.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Particles/BallisticParticleConstants.h>

xiiBallisticParticlePass::xiiBallisticParticlePass()  = default;
xiiBallisticParticlePass::~xiiBallisticParticlePass() = default;

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBallisticParticlePass,1,xiiRTTIDefaultAllocator<xiiBallisticParticlePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Seeds",m_Seeds)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Batches",m_Batches)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Particles",m_Particles)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Acceleration",m_Acceleration)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0,0,-9.81f))),
    XII_MEMBER_PROPERTY("MaximumParticles",m_uiMaximumParticles)->AddAttributes(new xiiDefaultValueAttribute(1048576U)),
    XII_MEMBER_PROPERTY("AllowAbsentSeeds",m_bAllowAbsentSeeds)->AddAttributes(new xiiDefaultValueAttribute(false)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES {new xiiCategoryAttribute("Rendering/Particles")} XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
void xiiBallisticParticlePass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  const auto first = out_pins.GetCount();
  xiiRenderPipelinePass::GetPins(out_pins);
  for (xiiUInt32 i = first; i < out_pins.GetCount(); ++i)
    out_pins[i].m_bOptional = m_bAllowAbsentSeeds;
}
xiiResult xiiBallisticParticlePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_Acceleration.IsValid() && m_uiMaximumParticles > 0 && m_uiMaximumParticles <= xiiParticleSystemConstants::s_uiMaxSupportedParticles ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiBallisticParticlePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto seeds = context.GetInputBuffer(m_Seeds), batches = context.GetInputBuffer(m_Batches);
  if (m_bAllowAbsentSeeds && !seeds.IsValid() && !batches.IsValid())
    return XII_SUCCESS;
  const auto* input      = context.m_Graph.GetBufferDescription(seeds);
  const auto* parameters = context.m_Graph.GetBufferDescription(batches);
  if (!input || input->m_Mode != xiiGALBufferMode::Structured || input->m_uiElementByteStride != sizeof(xiiParticleGPUState) || !input->m_uiSize || input->m_uiSize % sizeof(xiiParticleGPUState) != 0 || input->m_uiSize / sizeof(xiiParticleGPUState) > m_uiMaximumParticles || !parameters || parameters->m_Mode != xiiGALBufferMode::Structured || parameters->m_uiElementByteStride != sizeof(xiiParticleTrajectoryBatch) || !parameters->m_uiSize || parameters->m_uiSize % sizeof(xiiParticleTrajectoryBatch) != 0 || parameters->m_uiSize / sizeof(xiiParticleTrajectoryBatch) > xiiMath::MaxValue<xiiUInt32>())
    return context.ReportFailure("Ballistic particles require nonempty canonical birth and trajectory-batch buffers within the authored capacity.");
  if (!xiiGALDevice::HasDefaultDevice())
    return context.ReportFailure("Ballistic evaluation requires a graphics device.");
  const auto  count  = static_cast<xiiUInt32>(input->m_uiSize / sizeof(xiiParticleGPUState));
  const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  if (limits.m_uiMaxThreadGroupInvocations < 64 || limits.m_uiMaxThreadGroupSizeX < 64 || !limits.m_uiMaxThreadGroupCountX)
    return context.ReportFailure("Ballistic evaluation requires 64-thread compute groups.");
  const auto groups = (count + 63U) / 64U;
  const auto width  = xiiMath::Min(groups, xiiMath::Min(limits.m_uiMaxThreadGroupCountX, xiiMath::MaxValue<xiiUInt32>() / 64U));
  const auto height = (groups + width - 1) / width;
  if (height > limits.m_uiMaxThreadGroupCountY)
    return context.ReportFailure("Ballistic particle dispatch exceeds device limits.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/ParticleBallistic.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle               m_hSeeds, m_hBatches, m_hParticles, m_hConstants;
    xiiBallisticParticleConstants            m_Parameters;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiUInt32                                m_uiGroupsX, m_uiGroupsY;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_hSeeds=builder.ReadBuffer(seeds,xiiGALResourceStateFlags::ShaderResource);data.m_hBatches=builder.ReadBuffer(batches,xiiGALResourceStateFlags::ShaderResource);
    auto result=*input;result.m_BindFlags=xiiGALBindFlags::UnorderedAccess|xiiGALBindFlags::ShaderResource;result.m_Usage=xiiGALResourceUsage::Default;result.m_CPUAccessFlags=xiiGALCPUAccessFlag::None;
    data.m_hParticles=builder.WriteBuffer("Evaluated ballistic particles",result,xiiGALResourceStateFlags::UnorderedAccess);
    data.m_hConstants=xiiRenderPassUtils::Constants(builder,"Ballistic trajectory constants",sizeof(xiiBallisticParticleConstants));data.m_Parameters.Acceleration=xiiVec4(m_Acceleration.x,m_Acceleration.y,m_Acceleration.z,0);data.m_Parameters.Dispatch=xiiVec4U32(count,width*64U,0,0);data.m_uiGroupsX=width;data.m_uiGroupsY=height;data.m_Pipeline=m_Pipeline; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();{xiiGALMapHelper<xiiBallisticParticleConstants> mapped(commands,execute.GetBuffer(data.m_hConstants),xiiGALMapType::Write,xiiGALMapFlags::Discard);*mapped=data.m_Parameters;}
    commands.SetPipelineState(data.m_Pipeline.Borrow());commands.ResolveAndSetConstantBuffer("xiiBallisticParticleConstants",execute.GetBuffer(data.m_hConstants),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_Seeds",execute.GetBuffer(data.m_hSeeds)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceBufferView("g_Batches",execute.GetBuffer(data.m_hBatches)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_Particles",execute.GetBuffer(data.m_hParticles)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess),xiiGALShaderType::Compute);
    if(commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed()){execute.ReportFailure("Could not bind ballistic trajectory resources.");return;}
    commands.DispatchCompute({data.m_uiGroupsX,data.m_uiGroupsY,1}); });
  return context.SetOutput(m_Particles, emitted.first->m_hParticles);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Particles_Implementation_BallisticParticlePass);
