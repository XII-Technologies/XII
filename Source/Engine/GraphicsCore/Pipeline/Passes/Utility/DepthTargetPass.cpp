/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Utility/DepthTargetPass.h>
#include <GraphicsCore/Pipeline/View.h>
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDepthTargetPass, 1, xiiRTTIDefaultAllocator<xiiDepthTargetPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Reference", m_Reference)->AddFlags(xiiPropertyFlags::ReadOnly),
    XII_MEMBER_PROPERTY("DepthTarget", m_Depth),


    XII_MEMBER_PROPERTY("Depth", m_fDepth)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Utility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiDepthTargetPass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  return m_Depth.GetResolution().Validate().Succeeded() && xiiMath::IsFinite(m_fDepth) && m_fDepth >= 0 && m_fDepth <= 1 ? XII_SUCCESS : XII_FAILURE;
}
xiiResult xiiDepthTargetPass::Build(xiiRenderPipelinePassContext& context)
{
  xiiSizeU32 size;
  const auto index = m_Depth.GetBindingIndex();
  if (index >= context.m_Pins.GetCount())
    return context.ReportFailure("Depth target has no configured output pin.");
  auto output = context.m_Pins[index];
  if (output.m_Resolution.IsDefault() && (context.m_Parameters.Contains("RasterWidth") || context.m_Parameters.Contains("RasterHeight")))
  {
    XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "RasterWidth", output.m_Resolution.m_uiWidth));
    XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::ReadParameter(context, "RasterHeight", output.m_Resolution.m_uiHeight));
    output.m_Resolution.m_Mode = xiiRenderPipelineTextureResolutionMode::Fixed;
  }
  XII_SUCCEED_OR_RETURN(context.ResolveTextureSize(output, context.GetInputTexture(m_Reference), size));
  if (!size.width || !size.height || size.width > 65536 || size.height > 65536)
    return context.ReportFailure("Depth target requires a nonempty explicit, reference or view size.");
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiRenderGraphTextureHandle m_Depth;
    float                       m_fDepth;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    xiiGALTextureCreationDescription image; image.m_Type = xiiGALResourceDimension::Texture2D; image.m_Size = size;
    image.m_Format = xiiGALResourceFormat::D32Float; image.m_BindFlags = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
    data.m_Depth = builder.WriteTexture("Initialized Depth", image, xiiGALResourceStateFlags::DepthWrite); data.m_fDepth = m_fDepth;
    builder.SetPassRenderPassManaged(); }, [](const Data& data, xiiRenderGraphPassContext& execute) { xiiRenderPassUtils::ClearDepthPreservingState(execute.GetCommandList(), execute.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, data.m_fDepth, 0); });
  return context.SetOutput(m_Depth, emitted.first->m_Depth);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Utility_DepthTargetPass);
