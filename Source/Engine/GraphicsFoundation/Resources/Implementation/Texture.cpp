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

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTexture, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALTexture::xiiGALTexture(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALResource(pDevice), m_Description(creationDescription)
{
}

xiiGALTexture::~xiiGALTexture() = default;

xiiSharedPtr<xiiGALTextureView> xiiGALTexture::GetDefaultView(xiiEnum<xiiGALTextureViewType> viewType)
{
  XII_ASSERT_DEV(viewType > xiiGALTextureViewType::Undefined && viewType < xiiGALTextureViewType::ENUM_COUNT, "Invalid view type.");

  XII_ASSERT_DEV(m_DefaultTextureViews[viewType.GetValue()] != nullptr, "Texture view handle is invalid!");

  return m_DefaultTextureViews[viewType.GetValue()];
}

void xiiGALTexture::CreateDefaultResourceViews()
{
  // For texture cubes and texture cube arrays, we only address a single texture view per texture cube.
  xiiUInt32 uiArraySize = XII_GAL_REMAINING_ARRAY_SLICES;
  if (m_Description.IsCube() && m_Description.IsArray())
    uiArraySize = m_Description.GetArraySize() / 6U;

  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource))
  {
    xiiGALTextureViewCreationDescription viewDescription;
    viewDescription.m_ViewType                  = xiiGALTextureViewType::ShaderResource;
    viewDescription.m_pTexture                  = this;
    viewDescription.m_uiMostDetailedMip         = 0U;
    viewDescription.m_uiFirstArrayOrDepthSlice  = 0U;
    viewDescription.m_uiMipLevelCount           = XII_GAL_REMAINING_MIP_LEVELS;
    viewDescription.m_uiArrayOrDepthSlicesCount = XII_GAL_REMAINING_ARRAY_SLICES;

    if (m_Description.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::GenerateMips))
      viewDescription.m_Flags.Add(xiiGALTextureViewFlags::AllowMipGeneration);

    m_DefaultTextureViews[xiiGALTextureViewType::ShaderResource] = m_pDevice->CreateTextureView(viewDescription);
  }
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget))
  {
    xiiGALTextureViewCreationDescription viewDescription;
    viewDescription.m_ViewType                  = xiiGALTextureViewType::RenderTarget;
    viewDescription.m_pTexture                  = this;
    viewDescription.m_uiMostDetailedMip         = 0U;
    viewDescription.m_uiFirstArrayOrDepthSlice  = 0U;
    viewDescription.m_uiMipLevelCount           = XII_GAL_REMAINING_MIP_LEVELS;
    viewDescription.m_uiArrayOrDepthSlicesCount = uiArraySize;

    m_DefaultTextureViews[xiiGALTextureViewType::RenderTarget] = m_pDevice->CreateTextureView(viewDescription);
  }
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil))
  {
    xiiGALTextureViewCreationDescription viewDescription;
    viewDescription.m_ViewType                  = xiiGALTextureViewType::DepthStencil;
    viewDescription.m_pTexture                  = this;
    viewDescription.m_uiMostDetailedMip         = 0U;
    viewDescription.m_uiFirstArrayOrDepthSlice  = 0U;
    viewDescription.m_uiMipLevelCount           = XII_GAL_REMAINING_MIP_LEVELS;
    viewDescription.m_uiArrayOrDepthSlicesCount = uiArraySize;

    m_DefaultTextureViews[xiiGALTextureViewType::DepthStencil] = m_pDevice->CreateTextureView(viewDescription);

    viewDescription.m_ViewType                                         = xiiGALTextureViewType::ReadOnlyDepthStencil;
    m_DefaultTextureViews[xiiGALTextureViewType::ReadOnlyDepthStencil] = m_pDevice->CreateTextureView(viewDescription);
  }
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::UnorderedAccess))
  {
    xiiGALTextureViewCreationDescription viewDescription;
    viewDescription.m_ViewType                  = xiiGALTextureViewType::UnorderedAccess;
    viewDescription.m_pTexture                  = this;
    viewDescription.m_uiMostDetailedMip         = 0U;
    viewDescription.m_uiFirstArrayOrDepthSlice  = 0U;
    viewDescription.m_uiMipLevelCount           = XII_GAL_REMAINING_MIP_LEVELS;
    viewDescription.m_uiArrayOrDepthSlicesCount = uiArraySize;

    m_DefaultTextureViews[xiiGALTextureViewType::UnorderedAccess] = m_pDevice->CreateTextureView(viewDescription);
  }
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::ShadingRate))
  {
    xiiGALTextureViewCreationDescription viewDescription;
    viewDescription.m_ViewType                  = xiiGALTextureViewType::ShadingRate;
    viewDescription.m_pTexture                  = this;
    viewDescription.m_uiMostDetailedMip         = 0U;
    viewDescription.m_uiFirstArrayOrDepthSlice  = 0U;
    viewDescription.m_uiMipLevelCount           = XII_GAL_REMAINING_MIP_LEVELS;
    viewDescription.m_uiArrayOrDepthSlicesCount = uiArraySize;

    m_DefaultTextureViews[xiiGALTextureViewType::ShadingRate] = m_pDevice->CreateTextureView(viewDescription);
  }
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Texture);
