/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Lighting/LightingData.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/DirectionalSurfaceLightingPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/LightClusterPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/SurfaceLightingPassUtils.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDirectionalSurfaceLightingPass,1,xiiRTTIDefaultAllocator<xiiDirectionalSurfaceLightingPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Before", m_Before)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("After", m_After)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Depth",m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Normal",m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Albedo",m_Albedo)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Material",m_Material)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Emissive",m_Emissive)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera",m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Lighting",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("EnvironmentLight",m_Environment)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Visibility",m_Visibility)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Scale",m_fScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f),new xiiClampValueAttribute(0.0f,xiiVariant())),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiDirectionalSurfaceLightingPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fScale) && m_fScale >= 0 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiDirectionalSurfaceLightingPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiSurfaceLightingPassUtils::Inputs surface{context.GetInputTexture(m_Depth), context.GetInputTexture(m_Normal), context.GetInputTexture(m_Albedo), context.GetInputTexture(m_Material), context.GetInputTexture(m_Emissive), context.GetInputBuffer(m_Camera), {}};
  XII_SUCCEED_OR_RETURN(xiiSurfaceLightingPassUtils::Validate(context, surface));
  const auto  environment = context.GetInputBuffer(m_Environment);
  const auto* description = context.m_Graph.GetBufferDescription(environment);
  const auto  visibility  = context.GetInputTexture(m_Visibility);
  const auto* mask        = context.m_Graph.GetTextureDescription(visibility);
  if (!description || description->m_uiSize != sizeof(xiiEnvironmentLightConstants) || (visibility.IsValid() && (!mask || mask->m_Size != surface.m_Size || mask->m_uiArraySizeOrDepth != 1 || !xiiSurfaceLightingPassUtils::FloatReadable(mask->m_Format))))
    return context.ReportFailure("Directional lighting requires exact environment constants and matching optional visibility.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Lighting/DirectionalSurfaceLighting.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiSurfaceLightingPassUtils::Inputs      m_Surface;
    xiiRenderGraphTextureHandle              m_Output, m_Visibility;
    xiiRenderGraphBufferHandle               m_Constants, m_Environment;
    xiiSurfaceLightingConstants              m_Snapshot;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Surface=xiiSurfaceLightingPassUtils::Read(surface,builder); data.m_Pipeline=m_Pipeline; data.m_Output=xiiSurfaceLightingPassUtils::Output(builder,surface.m_Size);
    data.m_Snapshot.SurfaceExtent=xiiVec4U32(surface.m_Size.width,surface.m_Size.height,0,0); data.m_Snapshot.SurfaceEvaluation=xiiVec4(m_fScale,0,0,0);
    data.m_Constants=xiiRenderPassUtils::Constants(builder,"Surface Evaluation",sizeof(xiiSurfaceLightingConstants));
    data.m_Environment=builder.ReadBuffer(environment,xiiGALResourceStateFlags::ConstantBuffer); data.m_Visibility=visibility.IsValid() ? builder.ReadTexture(visibility,xiiGALResourceStateFlags::ShaderResource):data.m_Surface.m_Depth; data.m_Snapshot.SurfaceExtent.z=visibility.IsValid() ? 1U:0U; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList(); commands.SetPipelineState(data.m_Pipeline.Borrow()); xiiSurfaceLightingPassUtils::Bind(data.m_Surface,execute); xiiSurfaceLightingPassUtils::BindConstants(execute,data.m_Constants,data.m_Snapshot);
    commands.ResolveAndSetConstantBuffer("xiiEnvironmentLightConstants",execute.GetBuffer(data.m_Environment),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_DirectionalVisibility",execute.GetTexture(data.m_Visibility)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Lighting",execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);
    commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess(); commands.DispatchCompute({(data.m_Surface.m_Size.width+7U)/8U,(data.m_Surface.m_Size.height+7U)/8U,1}); });
  return context.SetOutput(m_Output, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_DirectionalSurfaceLightingPass);
