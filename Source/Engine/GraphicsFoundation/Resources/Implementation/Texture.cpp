#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

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

#define XII_GAL_TEXTURE_CHECK(expression, ...) \
  do                                           \
  {                                            \
    XII_ASSERT_DEV((expression), __VA_ARGS__); \
    if (!(expression)) { return {}; }          \
  } while (false)

xiiGALTexture::xiiGALTexture(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALResource(std::move(pDevice)), m_Description(creationDescription)
{
}

xiiGALTexture::~xiiGALTexture() = default;

xiiSharedPtr<xiiGALTextureView> xiiGALTexture::GetDefaultView(xiiEnum<xiiGALTextureViewType> viewType)
{
  XII_ASSERT_DEV(viewType > xiiGALTextureViewType::Undefined && viewType < xiiGALTextureViewType::ENUM_COUNT, "Invalid view type.");

  XII_ASSERT_DEV(m_DefaultTextureViews[viewType.GetValue()] != nullptr, "Texture view handle is invalid!");

  return m_DefaultTextureViews[viewType.GetValue()];
}

xiiSharedPtr<xiiGALTextureView> xiiGALTexture::CreateView(xiiGALTextureViewCreationDescription& description)
{
  XII_GAL_TEXTURE_CHECK(description.m_ViewType > xiiGALTextureViewType::Undefined && description.m_ViewType < xiiGALTextureViewType::ENUM_COUNT, "The texture view type is invalid.");
  XII_GAL_TEXTURE_CHECK(description.m_uiMostDetailedMip < m_Description.m_uiMipLevels, "The most detailed mip ({0}) is out of range. The texture has only {1} mip level (s).", description.m_uiMostDetailedMip, m_Description.m_uiMipLevels);
  XII_GAL_TEXTURE_CHECK((description.m_uiMipLevelCount == XII_GAL_REMAINING_MIP_LEVELS) || ((description.m_uiMostDetailedMip + description.m_uiMipLevelCount) <= m_Description.m_uiMipLevels), "The most detailed mip ({0}) and the number of mip levels in the view ({1}) is out of range. The texture has only {2} mip level (s).", description.m_uiMostDetailedMip, description.m_uiMipLevelCount, m_Description.m_uiMipLevels);

  if (description.m_Format == xiiGALResourceFormat::Unknown)
  {
    description.m_Format = xiiGALTextureUtilities::GetDefaultTextureViewFormat(m_Description.m_Format, description.m_ViewType, m_Description.m_BindFlags);
  }

  if (m_Description.IsArray())
  {
    XII_GAL_TEXTURE_CHECK(description.m_uiFirstArrayOrDepthSlice < m_Description.m_uiArraySizeOrDepth, "The first array slice ({0}) is out of range. The texture has only ({1}) slice (s)", description.m_uiFirstArrayOrDepthSlice, m_Description.m_uiArraySizeOrDepth);
    XII_GAL_TEXTURE_CHECK((description.m_uiArrayOrDepthSlicesCount == XII_GAL_REMAINING_ARRAY_SLICES) || ((description.m_uiFirstArrayOrDepthSlice + description.m_uiArrayOrDepthSlicesCount) <= m_Description.m_uiArraySizeOrDepth), "The first array slice ({0}) and the number of array slice (s) ({1}) are out of range. The texture has only ({2}) slice (s)", description.m_uiFirstArrayOrDepthSlice, description.m_uiArrayOrDepthSlicesCount, m_Description.m_uiArraySizeOrDepth);
  }
  else if (!m_Description.Is3D())
  {
    XII_GAL_TEXTURE_CHECK(description.m_uiFirstArrayOrDepthSlice == 0U, "For non-array texture, the First Array or Depth Slice must be zero.");
  }

  if (description.m_ResourceDimension == xiiGALResourceDimension::Undefined)
  {
    if (m_Description.m_Type == xiiGALResourceDimension::TextureCube || m_Description.m_Type == xiiGALResourceDimension::TextureCubeArray)
    {
      switch (description.m_ViewType)
      {
        case xiiGALTextureViewType::ShaderResource:
          description.m_ResourceDimension = m_Description.m_Type;
          break;

        case xiiGALTextureViewType::RenderTarget:
        case xiiGALTextureViewType::DepthStencil:
        case xiiGALTextureViewType::UnorderedAccess:
          description.m_ResourceDimension = xiiGALResourceDimension::Texture2DArray;
          break;

        default:
          XII_GAL_TEXTURE_CHECK(false, "Unexpected view type.");
      }
    }
    else
    {
      description.m_ResourceDimension = m_Description.m_Type;
    }
  }

  switch (m_Description.m_Type)
  {
    case xiiGALResourceDimension::Texture1D:
    {
      const bool bIsValid = description.m_ResourceDimension == xiiGALResourceDimension::Texture1D;
      XII_GAL_TEXTURE_CHECK(bIsValid, "Incorrect texture view type for a Texture 1D view, only xiiGALResourceDimension Texture1D is allowed.");
    }
    break;
    case xiiGALResourceDimension::Texture1DArray:
    {
      const bool bIsValid = description.m_ResourceDimension == xiiGALResourceDimension::Texture1D || description.m_ResourceDimension == xiiGALResourceDimension::Texture1DArray;
      XII_GAL_TEXTURE_CHECK(bIsValid, "Incorrect texture view type for a Texture 1D Array view, only xiiGALResourceDimension Texture1D or Texture1DArray is allowed.");
    }
    break;
    case xiiGALResourceDimension::Texture2D:
    {
      const bool bIsValid = description.m_ResourceDimension == xiiGALResourceDimension::Texture2D || description.m_ResourceDimension == xiiGALResourceDimension::Texture2DArray;
      XII_GAL_TEXTURE_CHECK(bIsValid, "Incorrect texture view type for a Texture 2D view, only xiiGALResourceDimension Texture2D or Texture2DArray is allowed.");
    }
    break;
    case xiiGALResourceDimension::Texture2DArray:
    {
      const bool bIsValid = description.m_ResourceDimension == xiiGALResourceDimension::Texture2D || description.m_ResourceDimension == xiiGALResourceDimension::Texture2DArray;
      XII_GAL_TEXTURE_CHECK(bIsValid, "Incorrect texture view type for a Texture 2D Array view, only xiiGALResourceDimension Texture2D or Texture2DArray is allowed.");
    }
    break;
    case xiiGALResourceDimension::Texture3D:
    {
      const bool bIsValid = description.m_ResourceDimension == xiiGALResourceDimension::Texture3D;
      XII_GAL_TEXTURE_CHECK(bIsValid, "Incorrect texture view type for a Texture 3D view, only xiiGALResourceDimension Texture3D is allowed.");
    }
    break;
    case xiiGALResourceDimension::TextureCube:
    {
      if (description.m_ViewType == xiiGALTextureViewType::ShaderResource)
      {
        const bool bIsValid = description.m_ResourceDimension == xiiGALResourceDimension::Texture2D || description.m_ResourceDimension == xiiGALResourceDimension::Texture2DArray || description.m_ResourceDimension == xiiGALResourceDimension::TextureCube;
        XII_GAL_TEXTURE_CHECK(bIsValid, "Incorrect texture view type for a Texture Cube shader resource view, only xiiGALResourceDimension Texture2D or Texture2DArray or TextureCube is allowed.");
      }
      else
      {
        const bool bIsValid = description.m_ResourceDimension == xiiGALResourceDimension::Texture2D || description.m_ResourceDimension == xiiGALResourceDimension::Texture2DArray;
        XII_GAL_TEXTURE_CHECK(bIsValid, "Incorrect texture view type for a Texture Cube non-shader resource view, only xiiGALResourceDimension Texture2D or Texture2DArray is allowed.");
      }
    }
    break;
    case xiiGALResourceDimension::TextureCubeArray:
    {
      if (description.m_ViewType == xiiGALTextureViewType::ShaderResource)
      {
        const bool bIsValid = description.m_ResourceDimension == xiiGALResourceDimension::Texture2D || description.m_ResourceDimension == xiiGALResourceDimension::Texture2DArray || description.m_ResourceDimension == xiiGALResourceDimension::TextureCube || description.m_ResourceDimension == xiiGALResourceDimension::TextureCubeArray;
        XII_GAL_TEXTURE_CHECK(bIsValid, "Incorrect texture view type for a Texture Cube Array shader resource view, only xiiGALResourceDimension Texture2D or Texture2DArray or TextureCube TextureCubeArray is allowed.");
      }
      else
      {
        const bool bIsValid = description.m_ResourceDimension == xiiGALResourceDimension::Texture2D || description.m_ResourceDimension == xiiGALResourceDimension::Texture2DArray;
        XII_GAL_TEXTURE_CHECK(bIsValid, "Incorrect texture view type for a Texture Cube Array non-shader resource view, only xiiGALResourceDimension Texture2D or Texture2DArray is allowed.");
      }
    }
    break;

    default:
      XII_GAL_TEXTURE_CHECK(false, "Encountered an unexpected view type.");
  }

  switch (description.m_ResourceDimension)
  {
    case xiiGALResourceDimension::TextureCube:
    {
      XII_GAL_TEXTURE_CHECK(description.m_ViewType == xiiGALTextureViewType::ShaderResource, "Unexpected view type, a Shader Resource view is expected.");
      XII_GAL_TEXTURE_CHECK((description.m_uiArrayOrDepthSlicesCount == 6U) || (description.m_uiArrayOrDepthSlicesCount == 0U) || (description.m_uiArrayOrDepthSlicesCount == XII_GAL_REMAINING_ARRAY_SLICES), "Texture Cube Shader Resource view is expected to have 6 array slices. {0} are provided.", description.m_uiArrayOrDepthSlicesCount);
    }
    break;
    case xiiGALResourceDimension::TextureCubeArray:
    {
      XII_GAL_TEXTURE_CHECK(description.m_ViewType == xiiGALTextureViewType::ShaderResource, "Unexpected view type, a Shader Resource view is expected.");
      XII_GAL_TEXTURE_CHECK((description.m_uiArrayOrDepthSlicesCount == XII_GAL_REMAINING_ARRAY_SLICES) || ((description.m_uiArrayOrDepthSlicesCount % 6U) == 0U), "The number of slices in Texture Cube Array Shader Resource view is expected to be a multiple of 6. {0} are provided.", description.m_uiArrayOrDepthSlicesCount);
    }
    break;
    case xiiGALResourceDimension::Texture1D:
    case xiiGALResourceDimension::Texture2D:
    {
      XII_GAL_TEXTURE_CHECK((description.m_uiArrayOrDepthSlicesCount == XII_GAL_REMAINING_ARRAY_SLICES) || (description.m_uiArrayOrDepthSlicesCount <= 1U), "The number of slices in the view ({0}) must be 1 (or 0) for non-array Texture 1D/2D views.", description.m_uiArrayOrDepthSlicesCount);
    }
    break;
    case xiiGALResourceDimension::Texture1DArray:
    case xiiGALResourceDimension::Texture2DArray:
    {
    }
    break;
    case xiiGALResourceDimension::Texture3D:
    {
      const xiiUInt32 uiMipDepth = xiiMath::Max(m_Description.m_uiArraySizeOrDepth >> description.m_uiMostDetailedMip, 1U);

      XII_GAL_TEXTURE_CHECK((description.m_uiFirstArrayOrDepthSlice + description.m_uiArrayOrDepthSlicesCount) <= uiMipDepth, "The first depth slice ({0}) and the number of slices in the view description ({1}), specify more slices than the target 3D texture mip level has ({2}).", description.m_uiFirstArrayOrDepthSlice, description.m_uiArrayOrDepthSlicesCount, uiMipDepth);
    }
    break;

    default:
      XII_GAL_TEXTURE_CHECK(false, "Unexpected texture dimension.");
  }

  XII_GAL_TEXTURE_CHECK(!xiiGALTextureUtilities::GetResourceFormatProperties(description.m_Format).m_bIsTypeless, "The texture view format ({0}) cannot be typeless.", description.m_Format.GetValue());

  if (description.m_Flags.IsSet(xiiGALTextureViewFlags::AllowMipGeneration))
  {
    XII_GAL_TEXTURE_CHECK(m_Description.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::GenerateMips), "The xiiGALTextureViewFlags::AllowMipGeneration flag can only be set if the texture was created with the xiiGALMiscTextureFlags::GenerateMips flag.");
    XII_GAL_TEXTURE_CHECK(description.m_ViewType == xiiGALTextureViewType::ShaderResource, "The xiiGALTextureViewFlags::AllowMipGeneration flag can only used with the xiiGALTextureViewType::ShaderResource view type.");
  }

  if (description.m_ViewType == xiiGALTextureViewType::ShadingRate)
  {
    XII_GAL_TEXTURE_CHECK(m_Description.m_BindFlags.IsSet(xiiGALBindFlags::ShadingRate), "To create a xiiGALTextureViewType::ShadingRate, the texture must be created with the xiiGALBindFlags::ShadingRate flag.");
  }

  if (description.m_ViewType != xiiGALTextureViewType::ShaderResource && !xiiGALTextureUtilities::IsIdentityComponentMapping(description.m_ComponentSwizzle))
  {
    XII_GAL_TEXTURE_CHECK(false, "Non-identity texture component swizzle is only supported for Shader Resource views.");
  }

  if (description.m_uiMipLevelCount == 0U || description.m_uiMipLevelCount == XII_GAL_REMAINING_MIP_LEVELS)
  {
    if (description.m_ViewType == xiiGALTextureViewType::ShaderResource)
      description.m_uiMipLevelCount = m_Description.m_uiMipLevels - description.m_uiMostDetailedMip;
    else
      description.m_uiMipLevelCount = 1U;
  }

  if (description.m_uiArrayOrDepthSlicesCount == 0 || description.m_uiArrayOrDepthSlicesCount == XII_GAL_REMAINING_ARRAY_SLICES)
  {
    if (m_Description.IsArray())
    {
      description.m_uiArrayOrDepthSlicesCount = m_Description.m_uiArraySizeOrDepth - description.m_uiFirstArrayOrDepthSlice;
    }
    else if (m_Description.Is3D())
    {
      const xiiUInt32 uiMipDepth              = xiiMath::Max(m_Description.m_uiArraySizeOrDepth >> description.m_uiMostDetailedMip, 1U);
      description.m_uiArrayOrDepthSlicesCount = uiMipDepth - description.m_uiFirstArrayOrDepthSlice;
    }
    else
    {
      description.m_uiArrayOrDepthSlicesCount = 1U;
    }
  }

  return CreateViewPlatform(description);
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
    viewDescription.m_uiMostDetailedMip         = 0U;
    viewDescription.m_uiFirstArrayOrDepthSlice  = 0U;
    viewDescription.m_uiMipLevelCount           = XII_GAL_REMAINING_MIP_LEVELS;
    viewDescription.m_uiArrayOrDepthSlicesCount = XII_GAL_REMAINING_ARRAY_SLICES;

    if (m_Description.m_MiscFlags.IsSet(xiiGALMiscTextureFlags::GenerateMips))
      viewDescription.m_Flags.Add(xiiGALTextureViewFlags::AllowMipGeneration);

    m_DefaultTextureViews[xiiGALTextureViewType::ShaderResource] = CreateView(viewDescription);
  }
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::RenderTarget))
  {
    xiiGALTextureViewCreationDescription viewDescription;
    viewDescription.m_ViewType                  = xiiGALTextureViewType::RenderTarget;
    viewDescription.m_uiMostDetailedMip         = 0U;
    viewDescription.m_uiFirstArrayOrDepthSlice  = 0U;
    viewDescription.m_uiMipLevelCount           = XII_GAL_REMAINING_MIP_LEVELS;
    viewDescription.m_uiArrayOrDepthSlicesCount = uiArraySize;

    m_DefaultTextureViews[xiiGALTextureViewType::RenderTarget] = CreateView(viewDescription);
  }
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::DepthStencil))
  {
    xiiGALTextureViewCreationDescription viewDescription;
    viewDescription.m_ViewType                  = xiiGALTextureViewType::DepthStencil;
    viewDescription.m_uiMostDetailedMip         = 0U;
    viewDescription.m_uiFirstArrayOrDepthSlice  = 0U;
    viewDescription.m_uiMipLevelCount           = XII_GAL_REMAINING_MIP_LEVELS;
    viewDescription.m_uiArrayOrDepthSlicesCount = uiArraySize;

    m_DefaultTextureViews[xiiGALTextureViewType::DepthStencil] = CreateView(viewDescription);

    viewDescription.m_ViewType                                         = xiiGALTextureViewType::ReadOnlyDepthStencil;
    m_DefaultTextureViews[xiiGALTextureViewType::ReadOnlyDepthStencil] = CreateView(viewDescription);
  }
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::UnorderedAccess))
  {
    xiiGALTextureViewCreationDescription viewDescription;
    viewDescription.m_ViewType                  = xiiGALTextureViewType::UnorderedAccess;
    viewDescription.m_uiMostDetailedMip         = 0U;
    viewDescription.m_uiFirstArrayOrDepthSlice  = 0U;
    viewDescription.m_uiMipLevelCount           = XII_GAL_REMAINING_MIP_LEVELS;
    viewDescription.m_uiArrayOrDepthSlicesCount = uiArraySize;

    m_DefaultTextureViews[xiiGALTextureViewType::UnorderedAccess] = CreateView(viewDescription);
  }
  if (m_Description.m_BindFlags.IsSet(xiiGALBindFlags::ShadingRate))
  {
    xiiGALTextureViewCreationDescription viewDescription;
    viewDescription.m_ViewType                  = xiiGALTextureViewType::ShadingRate;
    viewDescription.m_uiMostDetailedMip         = 0U;
    viewDescription.m_uiFirstArrayOrDepthSlice  = 0U;
    viewDescription.m_uiMipLevelCount           = XII_GAL_REMAINING_MIP_LEVELS;
    viewDescription.m_uiArrayOrDepthSlicesCount = uiArraySize;

    m_DefaultTextureViews[xiiGALTextureViewType::ShadingRate] = CreateView(viewDescription);
  }
}

xiiUInt64 xiiGALTexture::GetMemoryConsumption() const
{
  auto& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(m_Description.m_Format);

  // This generic implementation is only an approximation, but it can be overridden by specific implementations to give an accurate memory consumption figure.
  xiiUInt64 uiMemory = xiiUInt64(m_Description.m_Size.width) * xiiUInt64(m_Description.m_Size.height) * xiiUInt64(m_Description.m_uiArraySizeOrDepth);
  uiMemory *= formatProperties.GetElementSize();
  uiMemory *= m_Description.m_uiSampleCount;

  // Also account for mip maps.
  if (m_Description.m_uiMipLevels > 1)
  {
    uiMemory += static_cast<xiiUInt64>((1.0 / 3.0) * uiMemory);
  }

  return uiMemory;
}

#undef XII_GAL_TEXTURE_CHECK

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Texture);
