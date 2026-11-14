/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Containers/StaticArray.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/ScreenSpacePassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Output/BloomPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Pipeline/Passes/Output/BloomConstants.h>

struct xiiBloomPass::Data
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;

  xiiRenderGraphTextureHandle m_hHDRIn; ///< ShaderResource in (upscaled HDR input).
  xiiRenderGraphTextureHandle m_hBloom; ///< UnorderedAccess out (bloom result).
  xiiRenderGraphBufferHandle  m_hConstants;
  xiiRenderGraphTextureHandle m_hLowBloom;
  xiiUInt32                   m_uiStage = 0;
  xiiStringView               m_sOutputName; ///< Borrows one of the static stage names below.
  xiiSizeU32                  m_Size;
  xiiDisplayOutputSettings    m_DisplaySettings;
};
struct xiiBloomPass::State
{
  xiiSharedPtr<xiiGALComputePipelineState> m_Pipeline;
  xiiRenderPassUtils::DisplayConfiguration m_Display;
};

xiiBloomPass::xiiBloomPass()
{
  m_OutputBloomTexture.SetResolution({xiiRenderPipelineTextureResolutionMode::Input});
}
xiiBloomPass::~xiiBloomPass() = default;
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBloomPass, 2, xiiRTTIDefaultAllocator<xiiBloomPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("UpscaledColor", m_InputUpscaledColor)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("BloomTexture", m_OutputBloomTexture)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("UseViewDefaults", m_bUseViewDefaults)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new xiiDefaultValueAttribute(0.05f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Threshold", m_fThreshold)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("Knee", m_fKnee)->AddAttributes(new xiiDefaultValueAttribute(0.5f)),
    XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Output")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


xiiResult xiiBloomPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  xiiDisplayOutputSettings settings;
  settings.m_fBloomStrength  = m_fStrength;
  settings.m_fBloomThreshold = m_fThreshold;
  settings.m_fBloomKnee      = m_fKnee;
  settings.m_fBloomRadius    = m_fRadius;
  if (!xiiDisplayOutputManager::IsValid(settings))
    return XII_FAILURE;
  if (m_pState)
    m_pState->m_Display.m_bValid = false;
  return XII_SUCCESS;
}
xiiResult xiiBloomPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  if (!m_pState)
    m_pState = XII_DEFAULT_NEW(State);
  xiiDisplayOutputSettings settings;
  settings.m_fBloomStrength  = m_fStrength;
  settings.m_fBloomThreshold = m_fThreshold;
  settings.m_fBloomKnee      = m_fKnee;
  settings.m_fBloomRadius    = m_fRadius;
  XII_SUCCEED_OR_RETURN(m_pState->m_Display.Resolve(context, settings, m_bUseViewDefaults));
  if (m_pState->m_Display.m_Settings.m_fBloomStrength == 0 && (!context.m_pView || context.m_pView->GetViewRenderMode() != xiiViewRenderMode::Bloom))
    return XII_SUCCESS;
  if (!xiiScreenSpacePassUtils::Image(context, context.GetInputTexture(m_InputUpscaledColor), 4))
    return context.ReportFailure("Bloom requires readable single-sample floating-point RGBA input.");
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::LoadComputePipeline(m_pState->m_Pipeline, "Shaders/Pipeline/BloomChain.xiiShader"));
  xiiSizeU32 size;
  XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::OutputSize(context, m_OutputBloomTexture, context.GetInputTexture(m_InputUpscaledColor), size));
  const auto& limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties;
  if ((size.width + 7ULL) / 8 > limits.m_uiMaxThreadGroupCountX || (size.height + 7ULL) / 8 > limits.m_uiMaxThreadGroupCountY)
    return context.ReportFailure("Bloom exceeds compute dispatch dimensions.");
  // Separate graph resources expose every dependency and allow transient aliasing.
  // Six levels bound work and allocation independently of the viewport dimensions.
  const auto emit = [&](xiiStringView name, xiiSizeU32 extent, xiiRenderGraphTextureHandle input, xiiRenderGraphTextureHandle low, xiiUInt32 stage) {
    return context.m_Graph.AddPass<Data>(name, xiiGALCommandQueueFlags::Compute, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Size = extent;
      data.m_DisplaySettings = m_pState->m_Display.m_Settings;
      data.m_hHDRIn = input;
      data.m_hLowBloom = low.IsValid() ? low : input;
      data.m_uiStage = stage;
      data.m_sOutputName = name;
      Setup(data, builder); }, [](const Data& data, xiiRenderGraphPassContext& execute) { Execute(data, execute); }).first->m_hBloom;
  };
  xiiStaticArray<xiiSizeU32, 6>                  extents;
  xiiStaticArray<xiiRenderGraphTextureHandle, 6> levels;
  static constexpr xiiStringView                 s_DownsampleNames[] = {"Bloom Downsample 1", "Bloom Downsample 2", "Bloom Downsample 3", "Bloom Downsample 4", "Bloom Downsample 5"};
  static constexpr xiiStringView                 s_UpsampleNames[]   = {"Bloom Upsample 0", "Bloom Upsample 1", "Bloom Upsample 2", "Bloom Upsample 3", "Bloom Upsample 4"};
  extents.PushBack(size);
  levels.PushBack(emit("Bloom Extract", size, context.GetInputTexture(m_InputUpscaledColor), {}, 0));
  if (m_pState->m_Display.m_Settings.m_fBloomRadius > 0)
    while (extents.GetCount() < extents.GetCapacity() && (size.width > 1 || size.height > 1))
    {
      size = {xiiMath::Max(1U, (size.width + 1) / 2), xiiMath::Max(1U, (size.height + 1) / 2)};
      levels.PushBack(emit(s_DownsampleNames[levels.GetCount() - 1], size, levels.PeekBack(), {}, 1));
      extents.PushBack(size);
    }
  auto result = levels.PeekBack();
  for (xiiUInt32 level = levels.GetCount() - 1; level > 0; --level)
  {
    result = emit(s_UpsampleNames[level - 1], extents[level - 1], levels[level - 1], result, 2);
  }
  return context.SetOutput(m_OutputBloomTexture, result);
}

void xiiBloomPass::Setup(Data& data, xiiRenderGraphBuilder& builder)
{
  data.m_Pipeline                = m_pState->m_Pipeline;
  const xiiUInt32 uiOutputWidth  = data.m_Size.width;
  const xiiUInt32 uiOutputHeight = data.m_Size.height;

  data.m_hHDRIn    = builder.ReadTexture(data.m_hHDRIn, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLowBloom = builder.ReadTexture(data.m_hLowBloom, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiOutputWidth;
  description.m_Size.height = uiOutputHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hBloom             = builder.WriteTexture(data.m_sOutputName, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiBloomConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer(xiiStringBuilder(data.m_sOutputName, " Constants"), constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
}

void xiiBloomPass::Execute(const Data& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiOutputWidth  = data.m_Size.width;
  const xiiUInt32    uiOutputHeight = data.m_Size.height;

  cmd.BeginDebugGroup("Bloom");
  {
    {
      xiiGALMapHelper<xiiBloomConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->BloomParameters = xiiVec4(data.m_DisplaySettings.m_fBloomThreshold, data.m_DisplaySettings.m_fBloomKnee, data.m_DisplaySettings.m_fBloomRadius, float(data.m_uiStage));
    }

    cmd.SetPipelineState(data.m_Pipeline.Borrow());

    cmd.ResolveAndSetConstantBuffer("xiiBloomConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRIn", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_LowBloom", context.GetTexture(data.m_hLowBloom)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_BloomOut", context.GetTexture(data.m_hBloom)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    if (cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).Failed())
    {
      context.ReportFailure("Could not bind bloom resources.");
      cmd.EndDebugGroup();
      return;
    }
    cmd.DispatchCompute({(uiOutputWidth + 7U) / 8U, (uiOutputHeight + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Output_BloomPass);
