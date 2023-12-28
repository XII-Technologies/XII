#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Texture.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALMiscTextureFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::GenerateMips),
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::Memoryless),
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::SparseAlias),
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::Subsampled),
XII_END_STATIC_REFLECTED_BITFLAGS;

// clang-format on

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTexture, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALTexture::xiiGALTexture(const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALResource(), m_Description(creationDescription)
{
}

xiiGALTexture::~xiiGALTexture() = default;

void xiiGALTexture::CreateDefaultResourceViews(xiiGALTextureHandle hTexture)
{
  xiiGALTextureViewCreationDescription viewDescription;
  viewDescription.m_hTexture                  = hTexture;
  viewDescription.m_Format                    = m_Description.m_Format;
  viewDescription.m_uiMostDetailedMip         = 0U;
  viewDescription.m_uiFirstArrayOrDepthSlice  = 0U;
  viewDescription.m_uiMipLevelCount           = 0U;
  viewDescription.m_uiArrayOrDepthSlicesCount = 0U;

  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
  {
    viewDescription.m_ViewType                                   = xiiGALTextureViewType::ShaderResource;
    m_DefaultTextureViews[xiiGALTextureViewType::ShaderResource] = m_pDevice->CreateTextureView(viewDescription);
  }
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget))
  {
    viewDescription.m_ViewType                                 = xiiGALTextureViewType::RenderTarget;
    m_DefaultTextureViews[xiiGALTextureViewType::RenderTarget] = m_pDevice->CreateTextureView(viewDescription);
  }
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil))
  {
    viewDescription.m_ViewType                                 = xiiGALTextureViewType::DepthStencil;
    m_DefaultTextureViews[xiiGALTextureViewType::DepthStencil] = m_pDevice->CreateTextureView(viewDescription);

    viewDescription.m_ViewType                                         = xiiGALTextureViewType::ReadOnlyDepthStencil;
    m_DefaultTextureViews[xiiGALTextureViewType::ReadOnlyDepthStencil] = m_pDevice->CreateTextureView(viewDescription);
  }
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::UnorderedAccess))
  {
    viewDescription.m_ViewType                                    = xiiGALTextureViewType::UnorderedAccess;
    m_DefaultTextureViews[xiiGALTextureViewType::UnorderedAccess] = m_pDevice->CreateTextureView(viewDescription);
  }
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::ShadingRate))
  {
    viewDescription.m_ViewType                                = xiiGALTextureViewType::ShadingRate;
    m_DefaultTextureViews[xiiGALTextureViewType::ShadingRate] = m_pDevice->CreateTextureView(viewDescription);
  }
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Texture);
