/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearTexturePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClearTexturePass, 1, xiiRTTIDefaultAllocator<xiiClearTexturePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Reference", m_InputReference)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Texture", m_OutputTexture),

    XII_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::MakeZero())),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Utility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiClearTexturePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  const auto format = m_OutputTexture.GetFormat();
  if (m_OutputTexture.GetResolution().Validate().Failed() || format == xiiGALResourceFormat::Unknown || format.GetValue() >= xiiGALResourceFormat::ENUM_COUNT || xiiGALResourceFormat::IsDepthFormat(format) || xiiGALResourceFormat::IsTypeless(format) || xiiGALResourceFormat::IsMultiplanar(format) || xiiGALTextureUtilities::GetResourceFormatProperties(format).IsCompressed() || !xiiMath::IsFinite(m_Color.r) || !xiiMath::IsFinite(m_Color.g) || !xiiMath::IsFinite(m_Color.b) || !xiiMath::IsFinite(m_Color.a))
    return XII_FAILURE;
  return XII_SUCCESS;
}
xiiResult xiiClearTexturePass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  xiiSizeU32                 size;
  XII_SUCCEED_OR_RETURN(context.ResolveTextureSize(m_OutputTexture, context.GetInputTexture(m_InputReference), size));
  struct Data
  {
    xiiRenderGraphTextureHandle m_Texture;
    xiiColor                    m_Color;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    xiiGALTextureCreationDescription description;
    description.m_Type = xiiGALResourceDimension::Texture2D;
    description.m_Size = size;
    description.m_Format = context.m_Pins[m_OutputTexture.GetBindingIndex()].m_Format;
    description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    data.m_Texture = builder.WriteTexture("Cleared image", description, xiiGALResourceStateFlags::RenderTarget);
    data.m_Color = m_Color; }, [](const Data& data, xiiRenderGraphPassContext& execute) { xiiRenderPassUtils::ClearRenderTargetPreservingState(execute.GetCommandList(), execute.GetTexture(data.m_Texture)->GetDefaultView(xiiGALTextureViewType::RenderTarget), data.m_Color); });
  return context.SetOutput(m_OutputTexture, emitted.first->m_Texture);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Utility_ClearTexturePass);
