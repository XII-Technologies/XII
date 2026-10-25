/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowAtlasPass.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShadowAtlasPass, 1, xiiRTTIDefaultAllocator<xiiShadowAtlasPass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("CascadeSnapshot", m_Cascades)->AddFlags(xiiPropertyFlags::ReadOnly),
      XII_MEMBER_PROPERTY("ShadowAtlas", m_Atlas)->AddFlags(xiiPropertyFlags::ReadOnly),
    } XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {new xiiCategoryAttribute("Rendering/Shadows")} XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiResult xiiShadowAtlasPass::Build(xiiRenderPipelinePassContext& context)
{
  const auto input = context.GetInputData(m_Cascades);
  if (!input)
    return XII_SUCCESS;
  const auto* cascades = xiiDynamicCast<const xiiShadowCascadeSnapshot*>(input.Borrow());
  if (!cascades || cascades->m_Cascades.IsEmpty() || !xiiGALDevice::HasDefaultDevice())
    return context.ReportFailure("Shadow atlas requires an active directional cascade set and device.");
  const auto       resolution = cascades->m_uiResolution;
  const xiiSizeU32 size(resolution * (cascades->m_Cascades.GetCount() > 1 ? 2U : 1U), resolution * (cascades->m_Cascades.GetCount() > 2 ? 2U : 1U));
  const auto&      limits = xiiGALDevice::GetDefaultDevice()->GetGraphicsDeviceAdapterProperties().m_TextureProperties;
  if (!size.HasNonZeroArea() || size.width > limits.m_uiMaxTexture2DDimension || size.height > limits.m_uiMaxTexture2DDimension)
    return context.ReportFailure("Directional shadow atlas exceeds the device's 2D image limit; reduce authored resolution.");
  xiiRenderPassResourceScope scope(context);
  const auto                 emitted = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
    xiiGALTextureCreationDescription description;
    description.m_Type = xiiGALResourceDimension::Texture2D;
    description.m_Size = size;
    description.m_Format = xiiGALResourceFormat::D32Float;
    description.m_BindFlags = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
    data = builder.WriteTexture("Directional Shadow Atlas", description, xiiGALResourceStateFlags::DepthWrite);
    builder.SetPassRenderPassManaged(); }, [](const auto& data, xiiRenderGraphPassContext& execute) { xiiRenderPassUtils::ClearDepthPreservingState(execute.GetCommandList(), execute.GetTexture(data)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0, 0); });
  return context.SetOutput(m_Atlas, *emitted.first);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Shadows_ShadowAtlasPass);
