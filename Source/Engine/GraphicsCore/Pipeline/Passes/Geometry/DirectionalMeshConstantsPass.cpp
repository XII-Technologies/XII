/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Geometry/DirectionalMeshConstantsPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Geometry/DirectionalMeshConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDirectionalMeshConstantsPass, 1, xiiRTTIDefaultAllocator<xiiDirectionalMeshConstantsPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ShadowMap", m_ShadowMap)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Cascades", m_Cascades)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Constants", m_Constants)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ShadowProjection", m_ShadowProjection)->AddAttributes(new xiiDefaultValueAttribute(xiiMat4::MakeIdentity())),
    XII_MEMBER_PROPERTY("SunDirectionIntensity", m_SunDirectionIntensity)->AddAttributes(new xiiDefaultValueAttribute(xiiVec4(0,0,-1,1))),
    XII_MEMBER_PROPERTY("AmbientColor", m_AmbientColor)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0.12f,0.15f,0.22f))),
    XII_MEMBER_PROPERTY("SelectionPulseStrength", m_fSelectionPulseStrength),
    XII_MEMBER_PROPERTY("ShadowWorldUnitsPerTexel", m_fShadowWorldUnitsPerTexel),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Geometry") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiDirectionalMeshConstantsPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_ShadowProjection.IsValid() && m_SunDirectionIntensity.IsValid() && !m_SunDirectionIntensity.GetAsVec3().IsZero() && m_SunDirectionIntensity.w >= 0 &&
      m_AmbientColor.IsValid() && m_AmbientColor.x >= 0 && m_AmbientColor.y >= 0 && m_AmbientColor.z >= 0 && xiiMath::IsFinite(m_fSelectionPulseStrength) && m_fSelectionPulseStrength >= 0 &&
      xiiMath::IsFinite(m_fShadowWorldUnitsPerTexel) && m_fShadowWorldUnitsPerTexel >= 0 ?
    XII_SUCCESS :
    XII_FAILURE;
}
xiiResult xiiDirectionalMeshConstantsPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiDirectionalMeshConstants snapshot{};
  auto                        shadowProjection = m_ShadowProjection;
  snapshot.SunDirectionIntensity               = m_SunDirectionIntensity;
  snapshot.AmbientColor                        = xiiVec4(m_AmbientColor.x, m_AmbientColor.y, m_AmbientColor.z, m_fSelectionPulseStrength);
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "DirectionalSun", snapshot.SunDirectionIntensity));
  auto       texel  = m_fShadowWorldUnitsPerTexel;
  const auto source = context.GetInputData(m_Cascades);
  if (source)
  {
    const auto cascades = source.Downcast<const xiiShadowCascadeSnapshot>();
    if (!cascades || cascades->m_Cascades.IsEmpty() || !cascades->m_uiResolution)
      return context.ReportFailure("Directional mesh constants require a nonempty supplied cascade snapshot.");
    shadowProjection = cascades->m_Cascades[0].m_mViewProjection;
    texel            = 2 * cascades->m_Cascades[0].m_fWorldRadius / cascades->m_uiResolution;
  }
  const auto* shadow = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_ShadowMap));
  if (!shadow || shadow->m_Type != xiiGALResourceDimension::Texture2D || shadow->m_Format != xiiGALResourceFormat::D32Float || shadow->m_uiSampleCount != 1 || !shadow->m_Size.HasNonZeroArea() ||
      !shadowProjection.IsValid() || !snapshot.SunDirectionIntensity.IsValid() || snapshot.SunDirectionIntensity.GetAsVec3().IsZero() || snapshot.SunDirectionIntensity.w < 0 || !xiiMath::IsFinite(texel) || texel < 0 || shadow->m_uiArraySizeOrDepth != 1 || !shadow->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
    return context.ReportFailure("Invalid directional mesh light, projection or shadow image.");
  snapshot.ShadowViewProjectionMatrix = shadowProjection;
  snapshot.ShadowTexelSize            = xiiVec4(1.0f / shadow->m_Size.width, 1.0f / shadow->m_Size.height, texel, 0);
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphBufferHandle  m_Buffer;
    xiiDirectionalMeshConstants m_Snapshot;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Buffer=xiiRenderPassUtils::Constants(builder,"Directional Mesh Constants",sizeof(snapshot)); data.m_Snapshot=snapshot; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();
    {
      xiiGALMapHelper<xiiDirectionalMeshConstants> mapped(commands,execute.GetBuffer(data.m_Buffer),xiiGALMapType::Write,xiiGALMapFlags::Discard);
      *mapped=data.m_Snapshot;
    }
    // Dynamic mapping can stage a transfer; restore the state declared by this operation.
    xiiGALStateTransitionDescription transition;
    transition.m_pResource=execute.GetBuffer(data.m_Buffer);
    transition.m_NewState=xiiGALResourceStateFlags::ConstantBuffer;
    transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState;
    commands.TransitionResourceStates(xiiMakeArrayPtr(&transition,1)); });
  return context.SetOutput(m_Constants, pass.first->m_Buffer);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Geometry_DirectionalMeshConstantsPass);
