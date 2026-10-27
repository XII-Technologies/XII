/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Particles/ParticleSystem.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Particles/ParticleBillboardPass.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Materials/GpuSurfaceMaterial.h>
#include <Shaders/Pipeline/Passes/Transparency/TransparentRasterConstants.h>

xiiParticleBillboardPass::xiiParticleBillboardPass()  = default;
xiiParticleBillboardPass::~xiiParticleBillboardPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBillboardPass,1,xiiRTTIDefaultAllocator<xiiParticleBillboardPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Particles",m_Particles)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Materials",m_Materials)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Camera",m_Camera)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Depth",m_Depth)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InputAccumulation",m_InputAccumulation)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InputRevealage",m_InputRevealage)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InputAdditive",m_InputAdditive)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InputModulation",m_InputModulation)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("InputHistoryTrust",m_InputHistoryTrust)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Accumulation",m_Accumulation)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Revealage",m_Revealage)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Additive",m_Additive)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("Modulation",m_Modulation)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("HistoryTrust",m_HistoryTrust)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("WeightDepthScale",m_fWeightDepthScale)->AddAttributes(new xiiDefaultValueAttribute(5.0f)),
    XII_MEMBER_PROPERTY("MaximumWeight",m_fMaximumWeight)->AddAttributes(new xiiDefaultValueAttribute(3000.0f)),
    XII_MEMBER_PROPERTY("ColorScale",m_fColorScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES {new xiiCategoryAttribute("Rendering/Particles")} XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiParticleBillboardPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return xiiMath::IsFinite(m_fWeightDepthScale) && m_fWeightDepthScale > 0 && xiiMath::IsFinite(m_fMaximumWeight) && m_fMaximumWeight >= 0.01f && m_fMaximumWeight <= 1000000 && xiiMath::IsFinite(m_fColorScale) && m_fColorScale >= 0 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiParticleBillboardPass::Build(xiiRenderPipelinePassContext& context)
{
  const xiiRenderPipelineTextureInputPin*  inputs[]  = {&m_InputAccumulation, &m_InputRevealage, &m_InputAdditive, &m_InputModulation, &m_InputHistoryTrust};
  const xiiRenderPipelineTextureOutputPin* outputs[] = {&m_Accumulation, &m_Revealage, &m_Additive, &m_Modulation, &m_HistoryTrust};
  xiiRenderGraphTextureHandle              layers[5];
  xiiUInt32                                present = 0;
  for (xiiUInt32 i = 0; i < 5; ++i)
  {
    layers[i] = context.GetInputTexture(*inputs[i]);
    if (layers[i].IsValid())
      ++present;
  }
  if (present != 0 && present != 5)
    return context.ReportFailure("Particle append requires all five existing transparent layers, or none.");
  const auto particles = context.GetInputBuffer(m_Particles), materials = context.GetInputBuffer(m_Materials);
  if (!particles.IsValid() && !materials.IsValid())
  {
    if (present)
      for (xiiUInt32 i = 0; i < 5; ++i)
        XII_SUCCEED_OR_RETURN(context.SetOutput(*outputs[i], layers[i]));
    return XII_SUCCESS;
  }
  const auto valid = [&](xiiRenderGraphBufferHandle handle, xiiUInt32 stride) {
    const auto* d = context.m_Graph.GetBufferDescription(handle);
    return d && d->m_Mode == xiiGALBufferMode::Structured && d->m_uiElementByteStride == stride && d->m_uiSize && d->m_uiSize % stride == 0;
  };
  if (!valid(particles, sizeof(xiiParticleGPUState)) || !valid(materials, sizeof(xiiGpuSurfaceMaterial)))
    return context.ReportFailure("Particle raster requires canonical nonempty particle and compact material buffers.");
  const auto count = context.m_Graph.GetBufferDescription(particles)->m_uiSize / sizeof(xiiParticleGPUState);
  if (count > xiiParticleSystemConstants::s_uiMaxSupportedParticles)
    return context.ReportFailure("Billboard particle count exceeds supported capacity.");
  if (!context.m_pView || !xiiGALDevice::HasDefaultDevice())
    return context.ReportFailure("Billboards require a viewport and graphics device.");
  const auto device = xiiGALDevice::GetDefaultDevice();
  if (device->GetFeatures().m_IndependentBlend != xiiGALDeviceFeatureState::Enabled || device->GetGraphicsDeviceAdapterProperties().m_DeviceLimits.m_uiMaxRenderTargets < 5)
    return context.ReportFailure("Particle layers require independent blending and five render targets; disable the authored particle upload on unsupported devices.");
  const auto       depth  = context.GetInputTexture(m_Depth);
  const auto       camera = context.GetInputBuffer(m_Camera);
  const xiiSizeU32 size(context.m_pView->GetRenderResolutionWidth(), context.m_pView->GetRenderResolutionHeight());
  const auto*      d = xiiScreenSpacePassUtils::Image(context, depth, 1, &size);
  if (!d || d->m_Format != xiiGALResourceFormat::D32Float || !d->m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil) || !xiiScreenSpacePassUtils::Uniform(context, camera, sizeof(xiiFrameCameraConstants)))
    return context.ReportFailure("Billboards require viewport D32 readable depth and an exact camera snapshot.");
  for (xiiUInt32 i = 0; i < present; ++i)
  {
    const auto* layer = xiiScreenSpacePassUtils::Image(context, layers[i], i == 1 || i == 4 ? 1 : 4, &size);
    if (!layer || layer->m_Format != (i == 1 || i == 4 ? xiiGALResourceFormat::R32Float : xiiGALResourceFormat::RGBA32Float) || !layer->m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget))
      return context.ReportFailure("Particle append layers must match the canonical formats and viewport.");
  }
  if (!m_Shader.IsValid())
    m_Shader = xiiShaderPermutationUtilities::PreloadSinglePermutation(xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/ParticleBillboard.xiiShader"), {}, false);
  xiiResourceLock<xiiShaderPermutationResource> shader(m_Shader, xiiResourceAcquireMode::BlockTillLoaded);
  if (!shader.IsValid() || !shader->IsShaderValid())
    return context.ReportFailure("Particle billboard shader is unavailable.");
  if (!m_Sampler)
  {
    xiiGALSamplerCreationDescription s;
    s.m_MinFilter = xiiGALFilterType::Linear;
    s.m_MagFilter = xiiGALFilterType::Linear;
    s.m_MipFilter = xiiGALFilterType::Linear;
    s.m_AddressU  = xiiGALTextureAddressMode::Wrap;
    s.m_AddressV  = xiiGALTextureAddressMode::Wrap;
    m_Sampler     = device->CreateSampler(s);
  }
  if (!m_Sampler)
    return context.ReportFailure("Particle fallback sampler creation failed.");
  xiiRenderPassResourceScope scope(context);
  // GAL attachment states are exclusive. A distinct read image avoids sampling an attached depth image.
  struct DepthCopy
  {
    xiiRenderGraphTextureHandle m_Source, m_Sampled;
  };
  const auto depthCopy = context.m_Graph.AddPass<DepthCopy>("Particle soft depth", xiiGALCommandQueueFlags::Transfer, [&](DepthCopy& data, xiiRenderGraphBuilder& builder) {
    auto description=*d;
    data.m_Source=builder.ReadTexture(depth,xiiGALResourceStateFlags::CopySource);
    data.m_Sampled=builder.WriteTexture("Particle sampled depth",description,xiiGALResourceStateFlags::CopyDestination); }, [](const DepthCopy& data, xiiRenderGraphPassContext& execute) { execute.GetCommandList().CopyTexture(execute.GetTexture(data.m_Source), execute.GetTexture(data.m_Sampled)); });
  struct Uniforms
  {
    xiiRenderGraphBufferHandle    m_Buffer;
    xiiTransparentRasterConstants m_Values;
  };
  const auto uniform = context.m_Graph.AddPass<Uniforms>("Particle layer settings", xiiGALCommandQueueFlags::Transfer, [&](Uniforms& data, xiiRenderGraphBuilder& builder) {data.m_Buffer=xiiRenderPassUtils::Constants(builder,"Particle layer constants",sizeof(data.m_Values));data.m_Values.TransparentWeightsAndScale=xiiVec4(m_fWeightDepthScale,m_fMaximumWeight,m_fColorScale,0);data.m_Values.TransparentShadowBias=xiiVec4::MakeZero(); }, [](const Uniforms& data, xiiRenderGraphPassContext& execute) {xiiGALMapHelper<xiiTransparentRasterConstants> mapped(execute.GetCommandList(),execute.GetBuffer(data.m_Buffer),xiiGALMapType::Write,xiiGALMapFlags::Discard);*mapped=data.m_Values; });
  struct Data
  {
    xiiRenderGraphBufferHandle                    m_Particles, m_Materials, m_Camera, m_Uniform;
    xiiRenderGraphTextureHandle                   m_Depth, m_SampledDepth, m_Layers[5];
    xiiSharedPtr<xiiGALShader>                    m_Vertex, m_Pixel;
    xiiSharedPtr<xiiGALPipelineResourceSignature> m_Signature;
    xiiSharedPtr<xiiGALBlendState>                m_Blend;
    xiiSharedPtr<xiiGALDepthStencilState>         m_DepthState;
    xiiSharedPtr<xiiGALRasterizerState>           m_Rasterizer;
    xiiSharedPtr<xiiGALSampler>                   m_Sampler;
    xiiSizeU32                                    m_Size;
    xiiUInt32                                     m_uiCount;
    bool                                          m_bClear;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    builder.SetPassAllowMerge(false);data.m_Particles=builder.ReadBuffer(particles,xiiGALResourceStateFlags::ShaderResource);data.m_Materials=builder.ReadBuffer(materials,xiiGALResourceStateFlags::ShaderResource);data.m_Camera=builder.ReadBuffer(camera,xiiGALResourceStateFlags::ConstantBuffer);data.m_Uniform=builder.ReadBuffer(uniform.first->m_Buffer,xiiGALResourceStateFlags::ConstantBuffer);
    data.m_Depth=builder.ReadTexture(depth,xiiGALResourceStateFlags::DepthRead);data.m_SampledDepth=builder.ReadTexture(depthCopy.first->m_Sampled,xiiGALResourceStateFlags::ShaderResource);
    for(xiiUInt32 i=0;i<5;++i)
    {
      if(present)data.m_Layers[i]=builder.ReadWriteTexture(layers[i],xiiGALResourceStateFlags::RenderTarget);
      else{ xiiGALTextureCreationDescription target;target.m_Type=xiiGALResourceDimension::Texture2D;target.m_Size=size;target.m_Format=i==1||i==4?xiiGALResourceFormat::R32Float:xiiGALResourceFormat::RGBA32Float;target.m_BindFlags=xiiGALBindFlags::RenderTarget|xiiGALBindFlags::ShaderResource;xiiStringBuilder name;name.SetFormat("Particle layer {}",i);data.m_Layers[i]=builder.WriteTexture(name,target,xiiGALResourceStateFlags::RenderTarget);}
    }
    builder.SetPassRenderPassManaged();data.m_Vertex=shader->GetGALShader(xiiGALShaderType::Vertex);data.m_Pixel=shader->GetGALShader(xiiGALShaderType::Pixel);data.m_Signature=shader->GetPipelineResourceSignature();data.m_Blend=shader->GetBlendState();data.m_DepthState=shader->GetDepthStencilState();data.m_Rasterizer=shader->GetRasterizerState();data.m_Sampler=m_Sampler;data.m_Size=size;data.m_uiCount=static_cast<xiiUInt32>(count);data.m_bClear=present==0; }, [](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();
    if(data.m_bClear)for(xiiUInt32 i=0;i<5;++i)xiiRenderPassUtils::ClearRenderTargetPreservingState(commands,execute.GetTexture(data.m_Layers[i])->GetDefaultView(xiiGALTextureViewType::RenderTarget),i==1||i==3||i==4?xiiColor::White:xiiColor::MakeZero());
    xiiGALGraphicsPipelineStateCreationDescription p;p.m_PipelineType=xiiGALPipelineType::Graphics;p.m_pPipelineResourceSignature=data.m_Signature;p.m_pVertexShader=data.m_Vertex;p.m_pPixelShader=data.m_Pixel;p.m_GraphicsPipeline.m_pBlendState=data.m_Blend;p.m_GraphicsPipeline.m_pDepthStencilState=data.m_DepthState;p.m_GraphicsPipeline.m_pRasterizerState=data.m_Rasterizer;p.m_GraphicsPipeline.m_pRenderPass=execute.GetRenderPass();p.m_GraphicsPipeline.m_uiSubpassIndex=static_cast<xiiUInt8>(execute.GetSubpassIndex());p.m_GraphicsPipeline.m_PrimitiveTopology=xiiGALPrimitiveTopology::TriangleList;
    const auto state=xiiGALPipelineCache::GetPipeline(p);if(!state){execute.ReportFailure("Particle billboard pipeline creation failed, including required float attachment blending.");return;}
    commands.SetPipelineState(state.Borrow());commands.SetViewport({0,0,float(data.m_Size.width),float(data.m_Size.height),0,1});commands.ResolveAndSetConstantBuffer("PerFrameCamera",execute.GetBuffer(data.m_Camera),xiiGALShaderType::Vertex|xiiGALShaderType::Pixel);commands.ResolveAndSetConstantBuffer("xiiTransparentRasterConstants",execute.GetBuffer(data.m_Uniform),xiiGALShaderType::Pixel);
    commands.ResolveAndSetShaderResourceBufferView("g_Particles",execute.GetBuffer(data.m_Particles)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Vertex);commands.ResolveAndSetShaderResourceBufferView("g_SurfaceMaterials",execute.GetBuffer(data.m_Materials)->GetDefaultView(xiiGALBufferViewType::ShaderResource),xiiGALShaderType::Pixel);commands.ResolveAndSetShaderResourceTextureView("g_SceneDepth",execute.GetTexture(data.m_SampledDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource),xiiGALShaderType::Pixel);commands.ResolveAndSetSampler("g_sAnisotropicWrap",data.m_Sampler.Borrow());commands.ResolveAndSetSampler("g_sLinearWrap",data.m_Sampler.Borrow());
    xiiGALBindlessResourceTable::BindTextureSRVs(commands,"g_Textures",xiiGALShaderType::Pixel);xiiGALBindlessResourceTable::BindSamplers(commands,"g_MaterialSamplers",xiiGALShaderType::Pixel);
    if(commands.CommitShaderResources(xiiGALStateTransitionMode::Verify).Failed()){execute.ReportFailure("Could not bind particle billboard resources.");return;}
    xiiGALDrawDescription draw;draw.m_uiVertexCount=6;draw.m_uiInstanceCount=data.m_uiCount;commands.Draw(draw); });
  for (xiiUInt32 i = 0; i < 5; ++i)
    XII_SUCCEED_OR_RETURN(context.SetOutput(*outputs[i], emitted.first->m_Layers[i]));
  return XII_SUCCESS;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Particles_Implementation_ParticleBillboardPass);
