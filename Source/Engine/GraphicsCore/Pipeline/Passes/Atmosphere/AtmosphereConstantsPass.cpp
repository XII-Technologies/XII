/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/AtmospherePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAtmosphereConstantsPass, 1, xiiRTTIDefaultAllocator<xiiAtmosphereConstantsPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Before", m_Before)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("After", m_After)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("AtmosphereConstants", m_OutputAtmosphereConstants)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("UseWorldAtmosphere", m_bUseWorldAtmosphere)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("Settings", m_Settings),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Atmosphere")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


xiiResult xiiAtmosphereConstantsPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiIsValidAtmosphereSettings(m_Settings) ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiAtmosphereConstantsPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  auto                       settings = m_Settings;
  if (m_bUseWorldAtmosphere)
  {
    if (!context.m_pView)
      return XII_FAILURE;
    if (const auto* extracted = context.m_pView->GetExtractedRenderData())
      if (const auto* atmosphere = xiiSelectSkyAtmosphere(extracted->GetAllRenderData()))
        settings = atmosphere->m_AtmosphereSettings;
  }
  if (!xiiIsValidAtmosphereSettings(settings))
    return context.ReportFailure("Invalid authored or extracted atmosphere parameters.");
  struct Data
  {
    xiiAtmosphereConstants     m_Constants;
    xiiRenderGraphBufferHandle m_Buffer;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Constants = xiiRenderPassUtils::MakeAtmosphereConstants(settings);
      data.m_Buffer = xiiRenderPassUtils::Constants(builder, "AtmosphereConstants", sizeof(data.m_Constants)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      {
        xiiGALMapHelper<xiiAtmosphereConstants> mapped(commands, execute.GetBuffer(data.m_Buffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *mapped = data.m_Constants;
      }
      xiiGALStateTransitionDescription transition;
      transition.m_pResource = execute.GetBuffer(data.m_Buffer);
      transition.m_NewState = xiiGALResourceStateFlags::ConstantBuffer;
      transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
      commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1)); });
  return context.SetOutput(m_OutputAtmosphereConstants, emitted.first->m_Buffer);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Atmosphere_AtmosphereConstantsPass);
