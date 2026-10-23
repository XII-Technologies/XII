/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/EnvironmentLightConstantsPass.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEnvironmentLightConstantsPass, 1, xiiRTTIDefaultAllocator<xiiEnvironmentLightConstantsPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DirectionalLight", m_Directional)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("EnvironmentLight", m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Direction", m_Direction)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0,0,-1))),
    XII_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(1))),
    XII_MEMBER_PROPERTY("Illuminance", m_fIlluminance)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" lx")),
    XII_MEMBER_PROPERTY("AngularRadius", m_fAngularRadius)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("AmbientRadiance", m_Ambient)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0.03f,0.035f,0.04f))),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiEnvironmentLightConstantsPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_Direction.IsValid() && m_Direction.GetLengthSquared() > 1e-12f && m_Color.IsValid() && m_Color.x >= 0 && m_Color.y >= 0 && m_Color.z >= 0 &&
      m_Ambient.IsValid() && m_Ambient.x >= 0 && m_Ambient.y >= 0 && m_Ambient.z >= 0 && xiiMath::IsFinite(m_fIlluminance) && m_fIlluminance >= 0 &&
      xiiMath::IsFinite(m_fAngularRadius) && m_fAngularRadius >= 0 && m_fAngularRadius <= 1 ?
    XII_SUCCESS :
    XII_FAILURE;
}
xiiResult xiiEnvironmentLightConstantsPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiEnvironmentLightConstants snapshot{};
  auto                         direction = m_Direction;
  direction.NormalizeIfNotZero(xiiVec3(0, 0, -1)).AssertSuccess();
  snapshot.EnvironmentDirectionAndIlluminance = xiiVec4(direction, m_fIlluminance);
  snapshot.EnvironmentColorAndAngularRadius   = xiiVec4(m_Color, m_fAngularRadius);
  snapshot.EnvironmentAmbientRadiance         = xiiVec4(m_Ambient, 0);
  if (const auto supplied = context.GetInputData(m_Directional))
  {
    const auto* light = xiiDynamicCast<const xiiDirectionalLightFrameData*>(supplied.Borrow());
    if (!light)
      return context.ReportFailure("Environment illumination requires a directional-light snapshot.");
    snapshot.EnvironmentDirectionAndIlluminance = light->m_DirectionAndIlluminance;
    snapshot.EnvironmentColorAndAngularRadius   = light->m_ColorAndAngularRadius;
  }
  const auto& selectedDirection = snapshot.EnvironmentDirectionAndIlluminance;
  const auto& selectedColor     = snapshot.EnvironmentColorAndAngularRadius;
  if (!selectedDirection.IsValid() || selectedDirection.GetAsVec3().GetLengthSquared() < 1e-12f || selectedDirection.w < 0 || !selectedColor.IsValid() || selectedColor.x < 0 || selectedColor.y < 0 || selectedColor.z < 0 || selectedColor.w < 0 || selectedColor.w > 1)
    return context.ReportFailure("Invalid environment directional illumination snapshot.");
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle   m_Buffer;
    xiiEnvironmentLightConstants m_Snapshot;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) { data.m_Snapshot = snapshot; data.m_Buffer = xiiRenderPassUtils::Constants(builder, "Environment Illumination", sizeof(snapshot)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      { xiiGALMapHelper<xiiEnvironmentLightConstants> mapped(commands, execute.GetBuffer(data.m_Buffer), xiiGALMapType::Write, xiiGALMapFlags::Discard); *mapped = data.m_Snapshot; }
      xiiGALStateTransitionDescription transition;
      transition.m_pResource = execute.GetBuffer(data.m_Buffer); transition.m_NewState = xiiGALResourceStateFlags::ConstantBuffer; transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
      commands.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1)); });
  return context.SetOutput(m_Constants, pass.first->m_Buffer);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_EnvironmentLightConstantsPass);
