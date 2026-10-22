/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Particles/ParticleSystem.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Particles/ParticleIntegratePass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Particles/ParticleIntegrationConstants.h>

static_assert(sizeof(xiiParticleGPUState) == 128);
struct xiiParticleIntegratePass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
};
xiiParticleIntegratePass::xiiParticleIntegratePass()  = default;
xiiParticleIntegratePass::~xiiParticleIntegratePass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleIntegratePass, 1, xiiRTTIDefaultAllocator<xiiParticleIntegratePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ParticlesInput", m_InputParticles)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Particles")),
    XII_MEMBER_PROPERTY("Constants", m_InputConstants)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("ParticleSimConstants"), new xiiRenderPipelinePinOptionalAttribute("Bypass")),
    XII_MEMBER_PROPERTY("ParticlesOutput", m_OutputParticles)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("Particles")),
    XII_MEMBER_PROPERTY("ParticleCount", m_uiParticleCount)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, xiiParticleSystemConstants::s_uiMaxSupportedParticles)),
    XII_MEMBER_PROPERTY("Bypass", m_bBypass)->AddAttributes(new xiiDefaultValueAttribute(false)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Particles") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiParticleIntegratePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_uiParticleCount <= xiiParticleSystemConstants::s_uiMaxSupportedParticles ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiParticleIntegratePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  particles   = context.GetInputBuffer(m_InputParticles);
  const auto* description = context.m_Graph.GetBufferDescription(particles);
  if (!description || description->m_Mode != xiiGALBufferMode::Structured || description->m_uiElementByteStride != sizeof(xiiParticleGPUState) || description->m_uiSize % sizeof(xiiParticleGPUState) != 0 || description->m_uiSize / sizeof(xiiParticleGPUState) > xiiParticleSystemConstants::s_uiMaxSupportedParticles)
    return XII_FAILURE;
  if (m_bBypass)
    return context.SetOutput(m_OutputParticles, particles);
  const auto  constants  = context.GetInputBuffer(m_InputConstants);
  const auto* parameters = context.m_Graph.GetBufferDescription(constants);
  if (!parameters || parameters->m_uiSize < sizeof(xiiParticleIntegrationConstants))
    return XII_FAILURE;
  const auto capacity = static_cast<xiiUInt32>(description->m_uiSize / sizeof(xiiParticleGPUState));
  const auto count    = m_uiParticleCount ? m_uiParticleCount : capacity;
  if (count == 0 || count > capacity || !xiiGALDevice::HasDefaultDevice())
    return XII_FAILURE;
  const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  if (limits.m_uiMaxThreadGroupInvocations < 64 || limits.m_uiMaxThreadGroupSizeX < 64 || limits.m_uiMaxThreadGroupCountX == 0)
    return XII_FAILURE;
  const xiiUInt32 groups = (count + 63U) / 64U;
  const xiiUInt32 width  = xiiMath::Min(groups, limits.m_uiMaxThreadGroupCountX);
  const xiiUInt32 height = (groups + width - 1U) / width;
  if (height > limits.m_uiMaxThreadGroupCountY)
    return XII_FAILURE;
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/ParticleIntegrate.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle              m_Particles, m_Constants, m_Dispatch;
    xiiParticleIntegrationDispatchConstants m_Parameters;
    xiiUInt32                               m_uiGroupsY;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
    // Simulation changes state owned outside this frame, even when it is not rendered now.
    builder.SetPassSideEffects(true);
    data.m_Particles = builder.WriteBuffer(builder.ReadBuffer(particles, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
    data.m_Constants = builder.ReadBuffer(constants, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Dispatch = xiiRenderPassUtils::Constants(builder, "ParticleIntegrationDispatch", sizeof(data.m_Parameters));
    data.m_Parameters.DispatchGroupsX = width;
    data.m_Parameters.DispatchParticleCount = count;
    data.m_Parameters._Padding = xiiVec2U32::MakeZero();
    data.m_uiGroupsY = height; }, [this](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    {
      xiiGALMapHelper<xiiParticleIntegrationDispatchConstants> mapped(commands, execute.GetBuffer(data.m_Dispatch), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *mapped = data.m_Parameters;
    }
    commands.SetPipelineState(m_pState->m_Pipeline);
    commands.ResolveAndSetConstantBuffer("xiiParticleIntegrationConstants", execute.GetBuffer(data.m_Constants), xiiGALShaderType::Compute);
    commands.ResolveAndSetConstantBuffer("xiiParticleIntegrationDispatchConstants", execute.GetBuffer(data.m_Dispatch), xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessBufferView("g_Particles", execute.GetBuffer(data.m_Particles)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    commands.DispatchCompute({data.m_Parameters.DispatchGroupsX, data.m_uiGroupsY, 1}); });
  return context.SetOutput(m_OutputParticles, emitted.first->m_Particles);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Particles_ParticleIntegratePass);
