/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Refraction/ScreenRefractionTracePass.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Refraction/ScreenRefractionConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScreenRefractionTracePass,1,xiiRTTIDefaultAllocator<xiiScreenRefractionTracePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Depth",m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Normal",m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Material",m_Material)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Emissive",m_Emissive)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Radiance",m_Radiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera",m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Refraction",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("PerturbScale",m_fPerturbScale)->AddAttributes(new xiiDefaultValueAttribute(0.05f),new xiiClampValueAttribute(0.0f,0.5f)),
    XII_MEMBER_PROPERTY("MaximumDistance",m_fMaximumDistance)->AddAttributes(new xiiDefaultValueAttribute(0.1f),new xiiClampValueAttribute(0.0f,0.5f)),
    XII_MEMBER_PROPERTY("Chromatic",m_fChromatic)->AddAttributes(new xiiDefaultValueAttribute(0.002f),new xiiClampValueAttribute(0.0f,0.1f)),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES {new xiiCategoryAttribute("Rendering/Refraction")} XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiScreenRefractionTracePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fPerturbScale) && m_fPerturbScale >= 0 && m_fPerturbScale <= 0.5f && xiiMath::IsFinite(m_fMaximumDistance) && m_fMaximumDistance >= 0 && m_fMaximumDistance <= 0.5f && xiiMath::IsFinite(m_fChromatic) && m_fChromatic >= 0 && m_fChromatic <= 0.1f ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiScreenRefractionTracePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  depth = context.GetInputTexture(m_Depth), normal = context.GetInputTexture(m_Normal), material = context.GetInputTexture(m_Material), emissive = context.GetInputTexture(m_Emissive), radiance = context.GetInputTexture(m_Radiance);
  const auto  camera = context.GetInputBuffer(m_Camera);
  const auto* d      = xiiScreenSpacePassUtils::Image(context, depth, 1);
  if (!d || !xiiScreenSpacePassUtils::Image(context, normal, 2, &d->m_Size) || !xiiScreenSpacePassUtils::Image(context, material, 4, &d->m_Size) || !xiiScreenSpacePassUtils::Image(context, emissive, 4, &d->m_Size) || !xiiScreenSpacePassUtils::Image(context, radiance, 3, &d->m_Size) || !xiiScreenSpacePassUtils::Uniform(context, camera, sizeof(xiiFrameCameraConstants)))
    return context.ReportFailure("ScreenRefractionTrace requires matching surface and radiance images and exact camera constants.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Refraction/ScreenRefractionTrace.xiiShader"));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LinearClampSampler(m_Linear));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Depth, m_Normal, m_Material, m_Emissive, m_Radiance, m_Output;
    xiiRenderGraphBufferHandle               m_Camera, m_Constants;
    xiiScreenRefractionConstants             m_Snapshot;
    xiiSizeU32                               m_Size;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiSharedPtr<xiiGALSampler>              m_Linear;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Depth=builder.ReadTexture(depth,xiiGALResourceStateFlags::ShaderResource);data.m_Normal=builder.ReadTexture(normal,xiiGALResourceStateFlags::ShaderResource);data.m_Material=builder.ReadTexture(material,xiiGALResourceStateFlags::ShaderResource);data.m_Emissive=builder.ReadTexture(emissive,xiiGALResourceStateFlags::ShaderResource);data.m_Radiance=builder.ReadTexture(radiance,xiiGALResourceStateFlags::ShaderResource);data.m_Camera=builder.ReadBuffer(camera,xiiGALResourceStateFlags::ConstantBuffer);data.m_Size=d->m_Size;data.m_Output=xiiCompositionPassUtils::Output(builder,data.m_Size,xiiGALResourceFormat::RGBA32Float);data.m_Constants=xiiRenderPassUtils::Constants(builder,"Screen refraction options",sizeof(xiiScreenRefractionConstants));data.m_Snapshot.RefractionOptions=xiiVec4(m_fPerturbScale,m_fMaximumDistance,m_fChromatic,0);data.m_Pipeline=m_Pipeline;data.m_Linear=m_Linear; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& cmd=execute.GetCommandList();{xiiGALMapHelper<xiiScreenRefractionConstants> mapped(cmd,execute.GetBuffer(data.m_Constants),xiiGALMapType::Write,xiiGALMapFlags::Discard);*mapped=data.m_Snapshot;}
    xiiGALStateTransitionDescription transition;transition.m_pResource=execute.GetBuffer(data.m_Constants);transition.m_NewState=xiiGALResourceStateFlags::ConstantBuffer;transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState;cmd.TransitionResourceStates(xiiMakeArrayPtr(&transition,1));cmd.SetPipelineState(data.m_Pipeline.Borrow());
    cmd.ResolveAndSetConstantBuffer("xiiScreenRefractionConstants",execute.GetBuffer(data.m_Constants),xiiGALShaderType::Compute);cmd.ResolveAndSetConstantBuffer("xiiSurfaceCameraConstants",execute.GetBuffer(data.m_Camera),xiiGALShaderType::Compute);cmd.ResolveAndSetSampler("LinearClampSampler",data.m_Linear.Borrow());
    cmd.ResolveAndSetShaderResourceTextureView("g_Depth",execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);cmd.ResolveAndSetShaderResourceTextureView("g_Normal",execute.GetTexture(data.m_Normal)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);cmd.ResolveAndSetShaderResourceTextureView("g_Material",execute.GetTexture(data.m_Material)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);cmd.ResolveAndSetShaderResourceTextureView("g_Emissive",execute.GetTexture(data.m_Emissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);cmd.ResolveAndSetShaderResourceTextureView("g_Radiance",execute.GetTexture(data.m_Radiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);cmd.ResolveAndSetUnorderedAccessTextureView("g_Refraction",execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();cmd.DispatchCompute({(data.m_Size.width+7U)/8U,(data.m_Size.height+7U)/8U,1}); });
  return context.SetOutput(m_Output, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Refraction_ScreenRefractionTracePass);
