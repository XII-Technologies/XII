/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereApplyPass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <Shaders/Pipeline/Passes/Atmosphere/AtmosphereConstants.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAtmosphereApplyPass,1,xiiRTTIDefaultAllocator<xiiAtmosphereApplyPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Depth",m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Radiance",m_Radiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Transmittance",m_Transmittance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("MultiScatter",m_MultiScatter)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera",m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("EnvironmentLight",m_Light)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Atmosphere",m_Atmosphere)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AtmosphereRadiance",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES {new xiiCategoryAttribute("Rendering/Atmosphere")} XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiAtmosphereApplyPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  depth = context.GetInputTexture(m_Depth), radiance = context.GetInputTexture(m_Radiance), transmittance = context.GetInputTexture(m_Transmittance), multiScatter = context.GetInputTexture(m_MultiScatter);
  const auto  camera = context.GetInputBuffer(m_Camera), light = context.GetInputBuffer(m_Light), atmosphere = context.GetInputBuffer(m_Atmosphere);
  const auto* d = xiiScreenSpacePassUtils::Image(context, depth, 1);
  if (!d || !xiiScreenSpacePassUtils::Image(context, radiance, 4, &d->m_Size) || !xiiScreenSpacePassUtils::Image(context, transmittance, 3) || !xiiScreenSpacePassUtils::Image(context, multiScatter, 3) || !xiiScreenSpacePassUtils::Uniform(context, camera, sizeof(xiiFrameCameraConstants)) || !xiiScreenSpacePassUtils::Uniform(context, light, sizeof(xiiEnvironmentLightConstants)) || !xiiScreenSpacePassUtils::Uniform(context, atmosphere, sizeof(xiiAtmosphereConstants)))
    return context.ReportFailure("AtmosphereApply requires matching depth/radiance images, readable RGB LUTs and exact camera, environment and atmosphere constants.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Atmosphere/AtmosphereApply.xiiShader"));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::PointClampSampler(m_Point));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LinearClampSampler(m_Linear));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Depth, m_Radiance, m_Transmittance, m_MultiScatter, m_Output;
    xiiRenderGraphBufferHandle               m_Camera, m_Light, m_Atmosphere;
    xiiSizeU32                               m_Size;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiSharedPtr<xiiGALSampler>              m_Point, m_Linear;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Depth=builder.ReadTexture(depth,xiiGALResourceStateFlags::ShaderResource);data.m_Radiance=builder.ReadTexture(radiance,xiiGALResourceStateFlags::ShaderResource);data.m_Transmittance=builder.ReadTexture(transmittance,xiiGALResourceStateFlags::ShaderResource);data.m_MultiScatter=builder.ReadTexture(multiScatter,xiiGALResourceStateFlags::ShaderResource);data.m_Camera=builder.ReadBuffer(camera,xiiGALResourceStateFlags::ConstantBuffer);data.m_Light=builder.ReadBuffer(light,xiiGALResourceStateFlags::ConstantBuffer);data.m_Atmosphere=builder.ReadBuffer(atmosphere,xiiGALResourceStateFlags::ConstantBuffer);data.m_Size=d->m_Size;data.m_Output=xiiCompositionPassUtils::Output(builder,data.m_Size,xiiGALResourceFormat::RGBA32Float);data.m_Pipeline=m_Pipeline;data.m_Point=m_Point;data.m_Linear=m_Linear; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& cmd=execute.GetCommandList();cmd.SetPipelineState(data.m_Pipeline.Borrow());cmd.ResolveAndSetConstantBuffer("xiiSurfaceCameraConstants",execute.GetBuffer(data.m_Camera),xiiGALShaderType::Compute);cmd.ResolveAndSetConstantBuffer("xiiEnvironmentLightConstants",execute.GetBuffer(data.m_Light),xiiGALShaderType::Compute);cmd.ResolveAndSetConstantBuffer("xiiAtmosphereConstants",execute.GetBuffer(data.m_Atmosphere),xiiGALShaderType::Compute);cmd.ResolveAndSetSampler("PointClampSampler",data.m_Point.Borrow());cmd.ResolveAndSetSampler("LinearClampSampler",data.m_Linear.Borrow());
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth",execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);cmd.ResolveAndSetShaderResourceTextureView("g_SceneRadiance",execute.GetTexture(data.m_Radiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);cmd.ResolveAndSetShaderResourceTextureView("g_Transmittance",execute.GetTexture(data.m_Transmittance)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);cmd.ResolveAndSetShaderResourceTextureView("g_MultiScatter",execute.GetTexture(data.m_MultiScatter)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);cmd.ResolveAndSetUnorderedAccessTextureView("g_AtmosphereOutput",execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();cmd.DispatchCompute({(data.m_Size.width+7U)/8U,(data.m_Size.height+7U)/8U,1}); });
  return context.SetOutput(m_Output, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Atmosphere_AtmosphereApplyPass);
