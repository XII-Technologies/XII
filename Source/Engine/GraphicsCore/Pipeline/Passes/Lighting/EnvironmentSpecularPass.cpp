/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Composition/CompositionPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/EnvironmentSpecularPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/Implementation/ReflectionProbePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/SurfaceLightingPassUtils.h>
#include <GraphicsFoundation/Device/Device.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEnvironmentSpecularPass,1,xiiRTTIDefaultAllocator<xiiEnvironmentSpecularPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Depth",m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Normal",m_Normal)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Albedo",m_Albedo)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Material",m_Material)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("ReflectionProbes", m_Probes)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("EnvironmentIrradiance", m_Irradiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("AllowAbsentRadiance", m_bAllowAbsentRadiance)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("EnvironmentRadiance",m_Radiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("BRDFLut",m_Lut)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Occlusion",m_Occlusion)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera",m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Lighting",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Scale",m_fScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f),new xiiClampValueAttribute(0.0f,xiiVariant())),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiEnvironmentSpecularPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fScale) && m_fScale >= 0 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiEnvironmentSpecularPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto probes              = context.GetInputData(m_Probes).Downcast<const xiiReflectionProbeFrameData>();
  const bool local               = probes && !probes->m_Textures.IsEmpty();
  const bool environmentLighting = local || (probes && probes->m_Sky);
  if (!environmentLighting && !context.GetInputTexture(m_Radiance).IsValid() && m_bAllowAbsentRadiance)
    return XII_SUCCESS;
  if (local && xiiGALDevice::GetDefaultDevice()->GetFeatures().m_BindlessResources != xiiGALDeviceFeatureState::Enabled)
    return context.ReportFailure("Local reflection probes require nonuniform resource-array support.");
  xiiSurfaceLightingPassUtils::Inputs surface{context.GetInputTexture(m_Depth), context.GetInputTexture(m_Normal), context.GetInputTexture(m_Albedo), context.GetInputTexture(m_Material), {}, context.GetInputBuffer(m_Camera), {}};
  XII_SUCCEED_OR_RETURN(xiiSurfaceLightingPassUtils::Validate(context, surface, false));
  const auto radiance = context.GetInputTexture(m_Radiance), lut = context.GetInputTexture(m_Lut), occlusion = context.GetInputTexture(m_Occlusion);
  const auto valid = [&](xiiRenderGraphTextureHandle image, xiiUInt32 channels, bool sameSize) {
    const auto* d = context.m_Graph.GetTextureDescription(image);
    return d && d->m_Size.HasNonZeroArea() && (!sameSize || d->m_Size == surface.m_Size) && d->m_Type == xiiGALResourceDimension::Texture2D && d->m_uiArraySizeOrDepth == 1 && d->m_uiSampleCount == 1 && d->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) && xiiSurfaceLightingPassUtils::FloatReadable(d->m_Format) && xiiGALTextureUtilities::GetComponentCount(d->m_Format) >= channels;
  };
  if ((!environmentLighting && !valid(radiance, 4, true)) || !valid(lut, 2, false) || (occlusion.IsValid() && !valid(occlusion, 1, true)))
    return context.ReportFailure("Environment specular requires matching radiance/occlusion and a float-readable two-channel BRDF lookup.");
  auto& pipeline = local ? m_ProbePipeline : environmentLighting ? m_SkyPipeline :
                                                                   m_Pipeline;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(pipeline, local ? "Shaders/Pipeline/Passes/Lighting/EnvironmentProbeSpecular.xiiShader" : environmentLighting ? "Shaders/Pipeline/Passes/Lighting/EnvironmentSkySpecular.xiiShader" :
                                                                                                                                                                                "Shaders/Pipeline/Passes/Lighting/EnvironmentSpecular.xiiShader"));
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LinearClampSampler(m_Linear));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiReflectionProbePassUtils::Bindings    m_Probes;
    xiiSurfaceLightingPassUtils::Inputs      m_Surface;
    xiiRenderGraphTextureHandle              m_Radiance, m_Lut, m_Occlusion, m_Output, m_Irradiance;
    xiiRenderGraphBufferHandle               m_Constants;
    xiiSurfaceLightingConstants              m_Snapshot;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
    xiiSharedPtr<xiiGALSampler>              m_Linear;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Surface=xiiSurfaceLightingPassUtils::Read(surface,builder); if (environmentLighting) data.m_Probes=xiiReflectionProbePassUtils::Read(probes,builder); else data.m_Radiance=builder.ReadTexture(radiance,xiiGALResourceStateFlags::ShaderResource); data.m_Lut=builder.ReadTexture(lut,xiiGALResourceStateFlags::ShaderResource); data.m_Occlusion=occlusion.IsValid() ? builder.ReadTexture(occlusion,xiiGALResourceStateFlags::ShaderResource):data.m_Surface.m_Depth;
    data.m_Output=xiiCompositionPassUtils::Output(builder,surface.m_Size,xiiGALResourceFormat::RGBA32Float); data.m_Pipeline=pipeline; data.m_Linear=m_Linear;
    if (environmentLighting)
    {
      xiiGALTextureCreationDescription image;
      image.m_Type      = xiiGALResourceDimension::Texture2D;
      image.m_Size      = surface.m_Size;
      image.m_Format    = xiiGALResourceFormat::RGBA32Float;
      image.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
      data.m_Irradiance = builder.WriteTexture("Local Environment Irradiance", image, xiiGALResourceStateFlags::UnorderedAccess);
    }
    data.m_Snapshot.SurfaceExtent=xiiVec4U32(surface.m_Size.width,surface.m_Size.height,occlusion.IsValid() ? 1:0,0); data.m_Snapshot.SurfaceEvaluation=xiiVec4(m_fScale,0,0,0); data.m_Constants=xiiRenderPassUtils::Constants(builder,"Environment BRDF evaluation",sizeof(xiiSurfaceLightingConstants)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList(); commands.SetPipelineState(data.m_Pipeline.Borrow());
    xiiSurfaceLightingPassUtils::Bind(data.m_Surface,execute); xiiSurfaceLightingPassUtils::BindConstants(execute,data.m_Constants,data.m_Snapshot);
    commands.ResolveAndSetSampler("LinearClampSampler",data.m_Linear.Borrow());
    if (data.m_Probes.m_Snapshot) xiiReflectionProbePassUtils::Bind(data.m_Probes, execute, xiiGALShaderType::Compute);
    else commands.ResolveAndSetShaderResourceTextureView("g_EnvironmentRadiance",execute.GetTexture(data.m_Radiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_BRDFLut",execute.GetTexture(data.m_Lut)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Occlusion",execute.GetTexture(data.m_Occlusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Lighting",execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);
    if (data.m_Irradiance.IsValid())
      commands.ResolveAndSetUnorderedAccessTextureView("g_ProbeIrradiance", execute.GetTexture(data.m_Irradiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
    {
      execute.ReportFailure("Could not bind environment specular resources.");
      return;
    }
    commands.DispatchCompute({(data.m_Surface.m_Size.width+7U)/8U,(data.m_Surface.m_Size.height+7U)/8U,1}); });
  if (environmentLighting)
    XII_SUCCEED_OR_RETURN(context.SetOutput(m_Irradiance, pass.first->m_Irradiance));
  return context.SetOutput(m_Output, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_EnvironmentSpecularPass);
