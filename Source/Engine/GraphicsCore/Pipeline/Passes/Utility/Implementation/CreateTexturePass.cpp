/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Utility/CreateTexturePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCreateTexturePass, 1, xiiRTTIDefaultAllocator<xiiCreateTexturePass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Texture", m_OutputTexture)->AddFlags(xiiPropertyFlags::ReadOnly)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Width", m_uiWidth)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 65536U)),
    XII_MEMBER_PROPERTY("Height", m_uiHeight)->AddAttributes(new xiiDefaultValueAttribute(0U), new xiiClampValueAttribute(0U, 65536U)),
    XII_ENUM_MEMBER_PROPERTY("Format", xiiGALResourceFormat, m_Format)->AddAttributes(new xiiDefaultValueAttribute(xiiGALResourceFormat::RGBA16Float)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Utility")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiCreateTexturePass::Configure(const xiiVariantDictionary& properties)
{
  XII_SUCCEED_OR_RETURN(xiiRenderPipelinePass::Configure(properties));

  if (m_uiWidth > 65536 || m_uiHeight > 65536 || m_Format == xiiGALResourceFormat::Unknown || m_Format.GetValue() >= xiiGALResourceFormat::ENUM_COUNT ||
      xiiGALResourceFormat::IsDepthFormat(m_Format) || xiiGALResourceFormat::IsTypeless(m_Format) || xiiGALResourceFormat::IsMultiplanar(m_Format) ||
      xiiGALTextureUtilities::GetResourceFormatProperties(m_Format).IsCompressed())
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiResult xiiCreateTexturePass::Build(xiiRenderPipelinePassContext& context)
{
  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Size.width  = m_uiWidth ? m_uiWidth : context.m_pView ? context.m_pView->GetRenderResolutionWidth() :
                                                                        0;
  description.m_Size.height = m_uiHeight ? m_uiHeight : context.m_pView ? context.m_pView->GetRenderResolutionHeight() :
                                                                          0;
  description.m_Format      = m_Format;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  if (!description.m_Size.width || !description.m_Size.height || description.m_Size.width > 65536 || description.m_Size.height > 65536)
    return context.ReportFailure("Texture creation requires a nonempty size of at most 65536 in each dimension.");

  xiiRenderPassResourceScope scope(context);
  // Declaration has no GPU work. The graph culls this empty pass and allocates the texture at its first writer.
  const auto emitted = context.m_Graph.AddPass<xiiRenderGraphTextureHandle>(context.m_sPassName, xiiGALCommandQueueFlags::Graphics, [&](xiiRenderGraphTextureHandle& hTexture, xiiRenderGraphBuilder& builder) { hTexture = builder.DeclareTexture("Texture", description); }, [](const xiiRenderGraphTextureHandle&, xiiRenderGraphPassContext&) {});

  return context.SetOutput(m_OutputTexture, *emitted.first);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Utility_Implementation_CreateTexturePass);
