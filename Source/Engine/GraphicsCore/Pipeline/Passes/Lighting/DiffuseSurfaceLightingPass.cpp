/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Composition/CompositionPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/DiffuseSurfaceLightingPass.h>
#include <GraphicsCore/Pipeline/Passes/Lighting/SurfaceLightingPassUtils.h>
#include <Shaders/Pipeline/Passes/Lighting/EnvironmentLightConstants.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDiffuseSurfaceLightingPass,2,xiiRTTIDefaultAllocator<xiiDiffuseSurfaceLightingPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Depth",m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Albedo",m_Albedo)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Material",m_Material)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Emissive",m_Emissive)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Irradiance",m_Irradiance)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Occlusion",m_Occlusion)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("EnvironmentLight",m_Environment)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Lighting",m_Output)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Scale",m_fScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f),new xiiClampValueAttribute(0.0f,xiiVariant())),
    XII_MEMBER_PROPERTY("AsyncCompute",m_bAsyncCompute),
    XII_MEMBER_PROPERTY("AddIrradiance",m_bAddIrradiance)->AddAttributes(new xiiDefaultValueAttribute(false)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Lighting") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiDiffuseSurfaceLightingPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fScale) && m_fScale >= 0 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiDiffuseSurfaceLightingPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto  depth = context.GetInputTexture(m_Depth), albedo = context.GetInputTexture(m_Albedo), material = context.GetInputTexture(m_Material), emissive = context.GetInputTexture(m_Emissive), irradiance = context.GetInputTexture(m_Irradiance), occlusion = context.GetInputTexture(m_Occlusion);
  const auto  environment = context.GetInputBuffer(m_Environment);
  const auto* d           = context.m_Graph.GetTextureDescription(depth);
  const auto* env         = context.m_Graph.GetBufferDescription(environment);
  if (!d || !d->m_Size.HasNonZeroArea() || !env || env->m_uiSize != sizeof(xiiEnvironmentLightConstants))
    return context.ReportFailure("Diffuse lighting requires nonempty depth and exact environment constants.");
  const auto size  = d->m_Size;
  const auto image = [&](xiiRenderGraphTextureHandle handle, xiiUInt32 channels) {
    const auto* description = context.m_Graph.GetTextureDescription(handle);
    return description && description->m_Size == size && description->m_Type == xiiGALResourceDimension::Texture2D && description->m_uiArraySizeOrDepth == 1 && description->m_uiSampleCount == 1 && description->m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) && xiiSurfaceLightingPassUtils::FloatReadable(description->m_Format) && xiiGALTextureUtilities::GetComponentCount(description->m_Format) >= channels;
  };
  if (!image(depth, 1) || !image(albedo, 4) || !image(material, 4) || !image(emissive, 4) || (irradiance.IsValid() && !image(irradiance, 4)) || (occlusion.IsValid() && !image(occlusion, 1)))
    return context.ReportFailure("Diffuse lighting requires matching float-readable single-sample surface and irradiance images.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_Pipeline, "Shaders/Pipeline/Passes/Lighting/DiffuseSurfaceLighting.xiiShader"));
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle              m_Depth, m_Albedo, m_Material, m_Emissive, m_Irradiance, m_Occlusion, m_Output;
    xiiRenderGraphBufferHandle               m_Environment, m_Constants;
    xiiSurfaceLightingConstants              m_Snapshot;
    xiiSizeU32                               m_Size;
    xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  };
  const auto pass = context.m_Graph.AddPass<Data>(context.m_sPassName, m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Depth=builder.ReadTexture(depth,xiiGALResourceStateFlags::ShaderResource); data.m_Albedo=builder.ReadTexture(albedo,xiiGALResourceStateFlags::ShaderResource); data.m_Material=builder.ReadTexture(material,xiiGALResourceStateFlags::ShaderResource); data.m_Emissive=builder.ReadTexture(emissive,xiiGALResourceStateFlags::ShaderResource);
    data.m_Irradiance=irradiance.IsValid() ? builder.ReadTexture(irradiance,xiiGALResourceStateFlags::ShaderResource):data.m_Albedo;
    data.m_Occlusion=occlusion.IsValid() ? builder.ReadTexture(occlusion,xiiGALResourceStateFlags::ShaderResource):data.m_Depth;
    data.m_Environment=builder.ReadBuffer(environment,xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Size=size; data.m_Output=xiiCompositionPassUtils::Output(builder,size,xiiGALResourceFormat::RGBA32Float); data.m_Pipeline=m_Pipeline;
    data.m_Snapshot.SurfaceExtent=xiiVec4U32(size.width,size.height,irradiance.IsValid() ? 1:0,occlusion.IsValid() ? 1:0); data.m_Snapshot.SurfaceEvaluation=xiiVec4(m_fScale,m_bAddIrradiance ? 1.0f : 0.0f,0,0);
    data.m_Constants=xiiRenderPassUtils::Constants(builder,"Diffuse evaluation",sizeof(xiiSurfaceLightingConstants)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList(); commands.SetPipelineState(data.m_Pipeline.Borrow());
    xiiSurfaceLightingPassUtils::BindConstants(execute,data.m_Constants,data.m_Snapshot);
    commands.ResolveAndSetConstantBuffer("xiiEnvironmentLightConstants",execute.GetBuffer(data.m_Environment),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_SurfaceDepth",execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_SurfaceAlbedo",execute.GetTexture(data.m_Albedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_SurfaceMaterial",execute.GetTexture(data.m_Material)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_SurfaceEmissive",execute.GetTexture(data.m_Emissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Irradiance",execute.GetTexture(data.m_Irradiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetShaderResourceTextureView("g_Occlusion",execute.GetTexture(data.m_Occlusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Compute);
    commands.ResolveAndSetUnorderedAccessTextureView("g_Lighting",execute.GetTexture(data.m_Output)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess),xiiGALShaderType::Compute);
    if(commands.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed()){execute.ReportFailure("Could not bind diffuse lighting resources.");return;} commands.DispatchCompute({(data.m_Size.width+7U)/8U,(data.m_Size.height+7U)/8U,1}); });
  return context.SetOutput(m_Output, pass.first->m_Output);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Lighting_DiffuseSurfaceLightingPass);
