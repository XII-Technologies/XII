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
    XII_MEMBER_PROPERTY("Texture", m_OutputTexture)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Width", m_uiWidth)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 65536U)),
    XII_MEMBER_PROPERTY("Height", m_uiHeight)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 65536U)),
    XII_MEMBER_PROPERTY("ResolutionScale", m_fResolutionScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0625f, 16.0f)),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiGALResourceFormat, m_Format)->AddAttributes(new xiiDefaultValueAttribute(xiiGALResourceFormat::RGBA16Float)),
    XII_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::MakeZero())),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Utility") } XII_END_ATTRIBUTES;
} XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
xiiResult xiiClearTexturePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));
  if (m_uiWidth > 65536 || m_uiHeight > 65536 || !xiiMath::IsFinite(m_fResolutionScale) || m_fResolutionScale < 0.0625f || m_fResolutionScale > 16 ||
      m_Format == xiiGALResourceFormat::Unknown || m_Format.GetValue() >= xiiGALResourceFormat::ENUM_COUNT ||
      xiiGALResourceFormat::IsDepthFormat(m_Format) || xiiGALResourceFormat::IsTypeless(m_Format) || xiiGALResourceFormat::IsMultiplanar(m_Format) || xiiGALTextureUtilities::GetResourceFormatProperties(m_Format).IsCompressed() ||
      !xiiMath::IsFinite(m_Color.r) || !xiiMath::IsFinite(m_Color.g) || !xiiMath::IsFinite(m_Color.b) || !xiiMath::IsFinite(m_Color.a))
    return XII_FAILURE;
  return XII_SUCCESS;
}
xiiResult xiiClearTexturePass::Build(xiiRenderPipelinePassContext& context)
{
  xiiRenderPassResourceScope scope(context);
  const auto*                reference = context.m_Graph.GetTextureDescription(context.GetInputTexture(m_InputReference));
  xiiSizeU32                 size{m_uiWidth, m_uiHeight};
  if (!size.width)
    size.width = reference ? reference->m_Size.width : context.m_pView ? context.m_pView->GetRenderResolutionWidth() :
                                                                         0;
  if (!size.height)
    size.height = reference ? reference->m_Size.height : context.m_pView ? context.m_pView->GetRenderResolutionHeight() :
                                                                           0;
  if (!size.width || !size.height)
    return XII_FAILURE;
  const double width = size.width * static_cast<double>(m_fResolutionScale), height = size.height * static_cast<double>(m_fResolutionScale);
  if (width > 65536 || height > 65536)
    return XII_FAILURE;
  size = {xiiMath::Max(1U, static_cast<xiiUInt32>(width)), xiiMath::Max(1U, static_cast<xiiUInt32>(height))};
  struct Data
  {
    xiiRenderGraphTextureHandle m_Texture;
    xiiColor                    m_Color;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
    xiiGALTextureCreationDescription description;
    description.m_Type = xiiGALResourceDimension::Texture2D;
    description.m_Size = size;
    description.m_Format = m_Format;
    description.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    data.m_Texture = builder.WriteTexture("Cleared image", description, xiiGALResourceStateFlags::RenderTarget);
    data.m_Color = m_Color; }, [](const Data& data, xiiRenderGraphPassContext& execute) { xiiRenderPassUtils::ClearRenderTargetPreservingState(execute.GetCommandList(), execute.GetTexture(data.m_Texture)->GetDefaultView(xiiGALTextureViewType::RenderTarget), data.m_Color); });
  return context.SetOutput(m_OutputTexture, emitted.first->m_Texture);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Utility_ClearTexturePass);
