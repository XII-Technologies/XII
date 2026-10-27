/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/GlobalIllumination/ScreenIrradianceTracePass.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/GlobalIllumination/ScreenIrradianceConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScreenIrradianceTracePass,2,xiiRTTIDefaultAllocator<xiiScreenIrradianceTracePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Depth",m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Normal",m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Radiance",m_Radiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera",m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Irradiance",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("RayLength",m_fRayLength)->AddAttributes(new xiiDefaultValueAttribute(2.0f),new xiiClampValueAttribute(0.0f,xiiVariant())),
    XII_MEMBER_PROPERTY("Thickness",m_fThickness)->AddAttributes(new xiiDefaultValueAttribute(0.15f),new xiiClampValueAttribute(0.001f,xiiVariant())),
    XII_MEMBER_PROPERTY("SurfaceBias",m_fSurfaceBias)->AddAttributes(new xiiDefaultValueAttribute(0.02f)),
    XII_MEMBER_PROPERTY("AllowAbsentDepth",m_bAllowAbsentDepth)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("SampleCount",m_uiSampleCount)->AddAttributes(new xiiDefaultValueAttribute(8U),new xiiClampValueAttribute(1U,32U)),
    XII_MEMBER_PROPERTY("StepCount",m_uiStepCount)->AddAttributes(new xiiDefaultValueAttribute(6U),new xiiClampValueAttribute(1U,64U)),
    XII_MEMBER_PROPERTY("SampleOffset",m_uiSampleOffset),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES {new xiiCategoryAttribute("Rendering/GlobalIllumination")} XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiScreenIrradianceTracePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fRayLength) && m_fRayLength >= 0 && m_fRayLength <= 100000 && xiiMath::IsFinite(m_fThickness) && m_fThickness >= 0.001f && m_fThickness <= 10000 &&
      xiiMath::IsFinite(m_fSurfaceBias) && m_fSurfaceBias >= 0 && m_fSurfaceBias <= 10 && m_uiSampleCount > 0 && m_uiSampleCount <= 32 && m_uiStepCount > 0 && m_uiStepCount <= 64 ?
    XII_SUCCESS :
    XII_FAILURE;
}
void xiiScreenIrradianceTracePass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& out_pins) const
{
  const auto first = out_pins.GetCount();
  xiiRenderPipelinePass::GetPins(out_pins);
  for (xiiUInt32 i = first; i < out_pins.GetCount(); ++i)
    if (out_pins[i].m_sName == "Depth" || out_pins[i].m_sName == "Radiance" || out_pins[i].m_sName == "Irradiance")
      out_pins[i].m_bOptional = m_bAllowAbsentDepth;
}
xiiResult xiiScreenIrradianceTracePass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  depth = context.GetInputTexture(m_Depth), normal = context.GetInputTexture(m_Normal), radiance = context.GetInputTexture(m_Radiance);
  if (!depth.IsValid() && m_bAllowAbsentDepth)
    return XII_SUCCESS;
  const auto  camera = context.GetInputBuffer(m_Camera);
  const auto* d      = xiiScreenSpacePassUtils::Image(context, depth, 1);
  if (!d || !xiiScreenSpacePassUtils::Image(context, normal, 2, &d->m_Size) || !xiiScreenSpacePassUtils::Image(context, radiance, 3, &d->m_Size) || !xiiScreenSpacePassUtils::Uniform(context, camera, sizeof(xiiFrameCameraConstants)))
    return context.ReportFailure("ScreenIrradianceTrace requires matching depth, normal and radiance images and exact camera constants.");
  const auto  size   = d->m_Size;
  const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  if ((size.width + 7ULL) / 8 > limits.m_uiMaxThreadGroupCountX || (size.height + 7ULL) / 8 > limits.m_uiMaxThreadGroupCountY)
    return context.ReportFailure("Screen irradiance exceeds compute dispatch dimensions.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/GlobalIllumination/ScreenIrradianceTrace.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Depth, m_Normal, m_Radiance, m_Output;
    xiiRenderGraphBufferHandle               m_Camera, m_Constants;
    xiiScreenIrradianceConstants             m_Snapshot;
    xiiSizeU32                               m_Size;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Depth=builder.ReadTexture(depth,xiiGALResourceStateFlags::ShaderResource);data.m_Normal=builder.ReadTexture(normal,xiiGALResourceStateFlags::ShaderResource);data.m_Radiance=builder.ReadTexture(radiance,xiiGALResourceStateFlags::ShaderResource);data.m_Camera=builder.ReadBuffer(camera,xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Size=size;data.m_Output=xiiCompositionPassUtils::Output(builder,data.m_Size,xiiGALResourceFormat::RGBA32Float);data.m_Constants=xiiRenderPassUtils::Constants(builder,"Screen irradiance options",sizeof(xiiScreenIrradianceConstants));
    data.m_Snapshot.TraceDistances=xiiVec4(m_fRayLength,m_fThickness,m_fSurfaceBias,0);data.m_Snapshot.TraceCounts=xiiVec4U32(m_uiSampleCount,m_uiStepCount,m_uiSampleOffset,0);data.m_Pipeline=m_Pipeline; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& cmd=execute.GetCommandList();{xiiGALMapHelper<xiiScreenIrradianceConstants> mapped(cmd,execute.GetBuffer(data.m_Constants),xiiGALMapType::Write,xiiGALMapFlags::Discard);*mapped=data.m_Snapshot;}
    xiiGALStateTransitionDescription transition;transition.m_pResource=execute.GetBuffer(data.m_Constants);transition.m_NewState=xiiGALResourceStateFlags::ConstantBuffer;transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState;cmd.TransitionResourceStates(xiiMakeArrayPtr(&transition,1));cmd.SetPipelineState(data.m_Pipeline.Borrow());
    cmd.ResolveAndSetConstantBuffer("xiiScreenIrradianceConstants",execute.GetBuffer(data.m_Constants),xiiGALShaderType::Compute);cmd.ResolveAndSetConstantBuffer("xiiSurfaceCameraConstants",execute.GetBuffer(data.m_Camera),xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Depth",execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);cmd.ResolveAndSetShaderResourceTextureView("g_Normal",execute.GetTexture(data.m_Normal)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);cmd.ResolveAndSetShaderResourceTextureView("g_Radiance",execute.GetTexture(data.m_Radiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Irradiance",execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);if(cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed()){execute.ReportFailure("Could not bind screen irradiance resources.");return;}cmd.DispatchCompute({(data.m_Size.width+7U)/8U,(data.m_Size.height+7U)/8U,1}); });
  return context.SetOutput(m_Output, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_GlobalIllumination_ScreenIrradianceTracePass);
