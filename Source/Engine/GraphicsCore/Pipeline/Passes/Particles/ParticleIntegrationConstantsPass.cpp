/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/World/World.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Particles/ParticleSystem.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Particles/ParticleIntegrationConstantsPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Particles/ParticleIntegrationConstants.h>

static_assert(sizeof(xiiParticleIntegrationConstants) == 32);
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleIntegrationConstantsPass, 1, xiiRTTIDefaultAllocator<xiiParticleIntegrationConstantsPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Constants", m_OutputConstants)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute(), new xiiRenderPipelinePinNameAttribute("ParticleSimConstants")),
    XII_MEMBER_PROPERTY("UseWorldTime", m_bUseWorldTime)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("DeltaTimeS", m_fDeltaTimeS)->AddAttributes(new xiiDefaultValueAttribute(1.0f / 60.0f), new xiiClampValueAttribute(0.0f, 1.0f / 15.0f)),
    XII_MEMBER_PROPERTY("GravityScale", m_fGravityScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(-10000.0f, 10000.0f)),
    XII_MEMBER_PROPERTY("DragCoefficient", m_fDragCoefficient)->AddAttributes(new xiiDefaultValueAttribute(0.05f), new xiiClampValueAttribute(0.0f, 10000.0f)),
    XII_MEMBER_PROPERTY("TurbulenceScale", m_fTurbulenceScale)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, 10000.0f)),
    XII_MEMBER_PROPERTY("WindVelocity", m_WindVelocity),
    XII_MEMBER_PROPERTY("ActiveCount", m_uiActiveCount)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, xiiParticleSystemConstants::s_uiMaxSupportedParticles)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Particles") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiParticleIntegrationConstantsPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  if (!xiiMath::IsFinite(m_fDeltaTimeS) || m_fDeltaTimeS < 0 || m_fDeltaTimeS > 1.0f / 15.0f || !xiiMath::IsFinite(m_fGravityScale) || xiiMath::Abs(m_fGravityScale) > 10000 || !xiiMath::IsFinite(m_fDragCoefficient) || m_fDragCoefficient < 0 || m_fDragCoefficient > 10000 || !xiiMath::IsFinite(m_fTurbulenceScale) || m_fTurbulenceScale < 0 || m_fTurbulenceScale > 10000 || !m_WindVelocity.IsValid() || m_uiActiveCount > xiiParticleSystemConstants::s_uiMaxSupportedParticles)
    return XII_FAILURE;
  return XII_SUCCESS;
}
xiiResult xiiParticleIntegrationConstantsPass::Build(xiiRenderPipelinePassContext& context)
{
  float delta = m_fDeltaTimeS;
  if (m_bUseWorldTime)
  {
    if (!context.m_pView || !context.m_pView->GetWorld())
      return XII_FAILURE;
    delta = xiiMath::Clamp(static_cast<float>(context.m_pView->GetWorld()->GetClock().GetTimeDiff().GetSeconds()), 0.0f, 1.0f / 15.0f);
  }
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle      m_Buffer;
    xiiParticleIntegrationConstants m_Constants;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Constants.DeltaTimeS = delta;
    data.m_Constants.GravityScale = m_fGravityScale;
    data.m_Constants.DragCoefficient = m_fDragCoefficient;
    data.m_Constants.TurbulenceScale = m_fTurbulenceScale;
    data.m_Constants.WindVelocity = m_WindVelocity;
    data.m_Constants.ActiveCount = m_uiActiveCount;
    data.m_Buffer = xiiRenderPassUtils::Constants(builder, "ParticleSimConstants", sizeof(data.m_Constants)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    {
      xiiGALMapHelper<xiiParticleIntegrationConstants> mapped(commands, execute.GetBuffer(data.m_Buffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *mapped = data.m_Constants;
    }
    xiiGALStateTransitionDescription transition;
    transition.m_pResource = execute.GetBuffer(data.m_Buffer);
    transition.m_NewState = xiiGALResourceStateFlags::ConstantBuffer;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1)); });
  return context.SetOutput(m_OutputConstants, emitted.first->m_Buffer);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Particles_ParticleIntegrationConstantsPass);
