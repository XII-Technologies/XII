/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Device/DeviceD3D12.h>
#include <GraphicsD3D12/Resources/TextureD3D12.h>
#include <GraphicsD3D12/Resources/TextureViewD3D12.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTextureViewD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  [[nodiscard]] bool IsTextureViewTypeValidForBindFlags(xiiEnum<xiiGALTextureViewType> viewType, xiiBitflags<xiiGALBindFlags> bindFlags)
  {
    switch (viewType)
    {
      case xiiGALTextureViewType::ShaderResource:
        return bindFlags.IsSet(xiiGALBindFlags::ShaderResource);

      case xiiGALTextureViewType::RenderTarget:
        return bindFlags.IsSet(xiiGALBindFlags::RenderTarget);

      case xiiGALTextureViewType::DepthStencil:
      case xiiGALTextureViewType::ReadOnlyDepthStencil:
        return bindFlags.IsSet(xiiGALBindFlags::DepthStencil);

      case xiiGALTextureViewType::UnorderedAccess:
        return bindFlags.IsSet(xiiGALBindFlags::UnorderedAccess);

      case xiiGALTextureViewType::ShadingRate:
        return bindFlags.IsSet(xiiGALBindFlags::ShadingRate);

      default:
        return false;
    }
  }

  [[nodiscard]] bool IsMultiSampleTexture(const xiiGALTextureCreationDescription& description)
  {
    return description.m_uiSampleCount > 1U;
  }

  [[nodiscard]] bool HasStencilComponent(xiiEnum<xiiGALResourceFormat> format)
  {
    switch (format)
    {
      case xiiGALResourceFormat::D24UNormalizedS8UInt:
      case xiiGALResourceFormat::D32FloatS8X24UInt:
      case xiiGALResourceFormat::X24TypelessG8UInt:
      case xiiGALResourceFormat::X32TypelessG8X24UInt:
        return true;

      default:
        return false;
    }
  }

  xiiResult BuildShaderResourceViewDescription(const xiiGALTextureViewCreationDescription& viewDescription, const xiiGALTextureCreationDescription& textureDescription, xiiGALTextureViewD3D12::ViewMetadata& inout_viewMetadata)
  {
    D3D12_SHADER_RESOURCE_VIEW_DESC& shaderResourceView = inout_viewMetadata.m_ShaderResourceView;

    shaderResourceView                         = {};
    shaderResourceView.Format                  = xiiD3D12TypeConversions::GetFormat(viewDescription.m_Format);
    shaderResourceView.Shader4ComponentMapping = xiiD3D12TypeConversions::GetShaderComponentMapping(viewDescription.m_ComponentSwizzle);

    const bool bIsMultiSampleTexture = IsMultiSampleTexture(textureDescription);

    switch (viewDescription.m_ResourceDimension)
    {
      case xiiGALResourceDimension::Texture1D:
        shaderResourceView.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE1D;
        shaderResourceView.Texture1D.MostDetailedMip     = viewDescription.m_uiMostDetailedMip;
        shaderResourceView.Texture1D.MipLevels           = viewDescription.m_uiMipLevelCount;
        shaderResourceView.Texture1D.ResourceMinLODClamp = 0.0f;
        break;

      case xiiGALResourceDimension::Texture1DArray:
        shaderResourceView.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        shaderResourceView.Texture1DArray.MostDetailedMip     = viewDescription.m_uiMostDetailedMip;
        shaderResourceView.Texture1DArray.MipLevels           = viewDescription.m_uiMipLevelCount;
        shaderResourceView.Texture1DArray.FirstArraySlice     = viewDescription.m_uiFirstArrayOrDepthSlice;
        shaderResourceView.Texture1DArray.ArraySize           = viewDescription.m_uiArrayOrDepthSlicesCount;
        shaderResourceView.Texture1DArray.ResourceMinLODClamp = 0.0f;
        break;

      case xiiGALResourceDimension::Texture2D:
        if (bIsMultiSampleTexture)
        {
          shaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
          shaderResourceView.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
          shaderResourceView.Texture2D.MostDetailedMip     = viewDescription.m_uiMostDetailedMip;
          shaderResourceView.Texture2D.MipLevels           = viewDescription.m_uiMipLevelCount;
          shaderResourceView.Texture2D.PlaneSlice          = 0U;
          shaderResourceView.Texture2D.ResourceMinLODClamp = 0.0f;
        }
        break;

      case xiiGALResourceDimension::Texture2DArray:
        if (bIsMultiSampleTexture)
        {
          shaderResourceView.ViewDimension                    = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
          shaderResourceView.Texture2DMSArray.FirstArraySlice = viewDescription.m_uiFirstArrayOrDepthSlice;
          shaderResourceView.Texture2DMSArray.ArraySize       = viewDescription.m_uiArrayOrDepthSlicesCount;
        }
        else
        {
          shaderResourceView.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
          shaderResourceView.Texture2DArray.MostDetailedMip     = viewDescription.m_uiMostDetailedMip;
          shaderResourceView.Texture2DArray.MipLevels           = viewDescription.m_uiMipLevelCount;
          shaderResourceView.Texture2DArray.FirstArraySlice     = viewDescription.m_uiFirstArrayOrDepthSlice;
          shaderResourceView.Texture2DArray.ArraySize           = viewDescription.m_uiArrayOrDepthSlicesCount;
          shaderResourceView.Texture2DArray.PlaneSlice          = 0U;
          shaderResourceView.Texture2DArray.ResourceMinLODClamp = 0.0f;
        }
        break;

      case xiiGALResourceDimension::Texture3D:
        shaderResourceView.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
        shaderResourceView.Texture3D.MostDetailedMip     = viewDescription.m_uiMostDetailedMip;
        shaderResourceView.Texture3D.MipLevels           = viewDescription.m_uiMipLevelCount;
        shaderResourceView.Texture3D.ResourceMinLODClamp = 0.0f;
        break;

      case xiiGALResourceDimension::TextureCube:
        shaderResourceView.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
        shaderResourceView.TextureCube.MostDetailedMip     = viewDescription.m_uiMostDetailedMip;
        shaderResourceView.TextureCube.MipLevels           = viewDescription.m_uiMipLevelCount;
        shaderResourceView.TextureCube.ResourceMinLODClamp = 0.0f;
        break;

      case xiiGALResourceDimension::TextureCubeArray:
        shaderResourceView.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        shaderResourceView.TextureCubeArray.MostDetailedMip     = viewDescription.m_uiMostDetailedMip;
        shaderResourceView.TextureCubeArray.MipLevels           = viewDescription.m_uiMipLevelCount;
        shaderResourceView.TextureCubeArray.First2DArrayFace    = viewDescription.m_uiFirstArrayOrDepthSlice;
        shaderResourceView.TextureCubeArray.NumCubes            = viewDescription.m_uiArrayOrDepthSlicesCount / 6U;
        shaderResourceView.TextureCubeArray.ResourceMinLODClamp = 0.0f;
        break;

      default:
        XII_REPORT_FAILURE("Unsupported D3D12 shader-resource texture dimension '{}'.", xiiArgEnum(viewDescription.m_ResourceDimension));
        return XII_FAILURE;
    }

    inout_viewMetadata.m_bHasShaderResourceView = true;
    return XII_SUCCESS;
  }

  xiiResult BuildUnorderedAccessViewDescription(const xiiGALTextureViewCreationDescription& viewDescription, const xiiGALTextureCreationDescription& textureDescription, xiiGALTextureViewD3D12::ViewMetadata& inout_viewMetadata)
  {
    if (IsMultiSampleTexture(textureDescription))
    {
      xiiLog::Error("D3D12 unordered access texture views are unsupported for multi-sampled textures.");
      return XII_FAILURE;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC& unorderedAccessView = inout_viewMetadata.m_UnorderedAccessView;
    unorderedAccessView                                   = {};
    unorderedAccessView.Format                            = xiiD3D12TypeConversions::GetFormat(viewDescription.m_Format);

    switch (viewDescription.m_ResourceDimension)
    {
      case xiiGALResourceDimension::Texture1D:
        unorderedAccessView.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE1D;
        unorderedAccessView.Texture1D.MipSlice = viewDescription.m_uiMostDetailedMip;
        break;

      case xiiGALResourceDimension::Texture1DArray:
        unorderedAccessView.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
        unorderedAccessView.Texture1DArray.MipSlice        = viewDescription.m_uiMostDetailedMip;
        unorderedAccessView.Texture1DArray.FirstArraySlice = viewDescription.m_uiFirstArrayOrDepthSlice;
        unorderedAccessView.Texture1DArray.ArraySize       = viewDescription.m_uiArrayOrDepthSlicesCount;
        break;

      case xiiGALResourceDimension::Texture2D:
        unorderedAccessView.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        unorderedAccessView.Texture2D.MipSlice   = viewDescription.m_uiMostDetailedMip;
        unorderedAccessView.Texture2D.PlaneSlice = 0U;
        break;

      case xiiGALResourceDimension::Texture2DArray:
        unorderedAccessView.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        unorderedAccessView.Texture2DArray.MipSlice        = viewDescription.m_uiMostDetailedMip;
        unorderedAccessView.Texture2DArray.FirstArraySlice = viewDescription.m_uiFirstArrayOrDepthSlice;
        unorderedAccessView.Texture2DArray.ArraySize       = viewDescription.m_uiArrayOrDepthSlicesCount;
        unorderedAccessView.Texture2DArray.PlaneSlice      = 0U;
        break;

      case xiiGALResourceDimension::Texture3D:
        unorderedAccessView.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
        unorderedAccessView.Texture3D.MipSlice    = viewDescription.m_uiMostDetailedMip;
        unorderedAccessView.Texture3D.FirstWSlice = viewDescription.m_uiFirstArrayOrDepthSlice;
        unorderedAccessView.Texture3D.WSize       = viewDescription.m_uiArrayOrDepthSlicesCount;
        break;

      default:
        XII_REPORT_FAILURE("Unsupported D3D12 unordered-access texture dimension '{}'.", xiiArgEnum(viewDescription.m_ResourceDimension));
        return XII_FAILURE;
    }

    inout_viewMetadata.m_bHasUnorderedAccessView = true;
    return XII_SUCCESS;
  }

  xiiResult BuildRenderTargetViewDescription(const xiiGALTextureViewCreationDescription& viewDescription, const xiiGALTextureCreationDescription& textureDescription, xiiGALTextureViewD3D12::ViewMetadata& inout_viewMetadata)
  {
    D3D12_RENDER_TARGET_VIEW_DESC& renderTargetView = inout_viewMetadata.m_RenderTargetView;
    renderTargetView                                = {};
    renderTargetView.Format                         = xiiD3D12TypeConversions::GetFormat(viewDescription.m_Format);

    const bool bIsMultiSampleTexture = IsMultiSampleTexture(textureDescription);

    switch (viewDescription.m_ResourceDimension)
    {
      case xiiGALResourceDimension::Texture1D:
        renderTargetView.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE1D;
        renderTargetView.Texture1D.MipSlice = viewDescription.m_uiMostDetailedMip;
        break;

      case xiiGALResourceDimension::Texture1DArray:
        renderTargetView.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
        renderTargetView.Texture1DArray.MipSlice        = viewDescription.m_uiMostDetailedMip;
        renderTargetView.Texture1DArray.FirstArraySlice = viewDescription.m_uiFirstArrayOrDepthSlice;
        renderTargetView.Texture1DArray.ArraySize       = viewDescription.m_uiArrayOrDepthSlicesCount;
        break;

      case xiiGALResourceDimension::Texture2D:
        if (bIsMultiSampleTexture)
        {
          renderTargetView.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
          renderTargetView.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
          renderTargetView.Texture2D.MipSlice   = viewDescription.m_uiMostDetailedMip;
          renderTargetView.Texture2D.PlaneSlice = 0U;
        }
        break;

      case xiiGALResourceDimension::Texture2DArray:
        if (bIsMultiSampleTexture)
        {
          renderTargetView.ViewDimension                    = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
          renderTargetView.Texture2DMSArray.FirstArraySlice = viewDescription.m_uiFirstArrayOrDepthSlice;
          renderTargetView.Texture2DMSArray.ArraySize       = viewDescription.m_uiArrayOrDepthSlicesCount;
        }
        else
        {
          renderTargetView.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
          renderTargetView.Texture2DArray.MipSlice        = viewDescription.m_uiMostDetailedMip;
          renderTargetView.Texture2DArray.FirstArraySlice = viewDescription.m_uiFirstArrayOrDepthSlice;
          renderTargetView.Texture2DArray.ArraySize       = viewDescription.m_uiArrayOrDepthSlicesCount;
          renderTargetView.Texture2DArray.PlaneSlice      = 0U;
        }
        break;

      case xiiGALResourceDimension::Texture3D:
        renderTargetView.ViewDimension         = D3D12_RTV_DIMENSION_TEXTURE3D;
        renderTargetView.Texture3D.MipSlice    = viewDescription.m_uiMostDetailedMip;
        renderTargetView.Texture3D.FirstWSlice = viewDescription.m_uiFirstArrayOrDepthSlice;
        renderTargetView.Texture3D.WSize       = viewDescription.m_uiArrayOrDepthSlicesCount;
        break;

      default:
        XII_REPORT_FAILURE("Unsupported D3D12 render-target texture dimension '{}'.", xiiArgEnum(viewDescription.m_ResourceDimension));
        return XII_FAILURE;
    }

    inout_viewMetadata.m_bHasRenderTargetView = true;
    return XII_SUCCESS;
  }

  xiiResult BuildDepthStencilViewDescription(const xiiGALTextureViewCreationDescription& viewDescription, const xiiGALTextureCreationDescription& textureDescription, xiiGALTextureViewD3D12::ViewMetadata& inout_viewMetadata)
  {
    if (viewDescription.m_ResourceDimension == xiiGALResourceDimension::Texture3D || viewDescription.m_ResourceDimension == xiiGALResourceDimension::TextureCube || viewDescription.m_ResourceDimension == xiiGALResourceDimension::TextureCubeArray)
    {
      xiiLog::Error("D3D12 depth-stencil views are unsupported for '{}' dimensions.", xiiArgEnum(viewDescription.m_ResourceDimension));
      return XII_FAILURE;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC& depthStencilView = inout_viewMetadata.m_DepthStencilView;
    depthStencilView                                = {};
    depthStencilView.Format                         = xiiD3D12TypeConversions::GetFormat(viewDescription.m_Format);

    if (viewDescription.m_ViewType == xiiGALTextureViewType::ReadOnlyDepthStencil)
    {
      depthStencilView.Flags |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;
      if (HasStencilComponent(viewDescription.m_Format))
      {
        depthStencilView.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;
      }
    }

    const bool bIsMultiSampleTexture = IsMultiSampleTexture(textureDescription);

    switch (viewDescription.m_ResourceDimension)
    {
      case xiiGALResourceDimension::Texture1D:
        depthStencilView.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE1D;
        depthStencilView.Texture1D.MipSlice = viewDescription.m_uiMostDetailedMip;
        break;

      case xiiGALResourceDimension::Texture1DArray:
        depthStencilView.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
        depthStencilView.Texture1DArray.MipSlice        = viewDescription.m_uiMostDetailedMip;
        depthStencilView.Texture1DArray.FirstArraySlice = viewDescription.m_uiFirstArrayOrDepthSlice;
        depthStencilView.Texture1DArray.ArraySize       = viewDescription.m_uiArrayOrDepthSlicesCount;
        break;

      case xiiGALResourceDimension::Texture2D:
        if (bIsMultiSampleTexture)
        {
          depthStencilView.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
          depthStencilView.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
          depthStencilView.Texture2D.MipSlice = viewDescription.m_uiMostDetailedMip;
        }
        break;

      case xiiGALResourceDimension::Texture2DArray:
        if (bIsMultiSampleTexture)
        {
          depthStencilView.ViewDimension                    = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
          depthStencilView.Texture2DMSArray.FirstArraySlice = viewDescription.m_uiFirstArrayOrDepthSlice;
          depthStencilView.Texture2DMSArray.ArraySize       = viewDescription.m_uiArrayOrDepthSlicesCount;
        }
        else
        {
          depthStencilView.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
          depthStencilView.Texture2DArray.MipSlice        = viewDescription.m_uiMostDetailedMip;
          depthStencilView.Texture2DArray.FirstArraySlice = viewDescription.m_uiFirstArrayOrDepthSlice;
          depthStencilView.Texture2DArray.ArraySize       = viewDescription.m_uiArrayOrDepthSlicesCount;
        }
        break;

      default:
        XII_REPORT_FAILURE("Unsupported D3D12 depth-stencil texture dimension '{}'.", xiiArgEnum(viewDescription.m_ResourceDimension));
        return XII_FAILURE;
    }

    inout_viewMetadata.m_bHasDepthStencilView = true;
    return XII_SUCCESS;
  }
} // namespace

xiiGALTextureViewD3D12::xiiGALTextureViewD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, xiiSharedPtr<xiiGALTexture> pTexture, const xiiGALTextureViewCreationDescription& creationDescription) :
  xiiGALTextureView(std::move(pDeviceD3D12), std::move(pTexture), creationDescription)
{
}

xiiGALTextureViewD3D12::~xiiGALTextureViewD3D12() = default;

xiiResult xiiGALTextureViewD3D12::InitPlatform()
{
  xiiSharedPtr<xiiGALTextureD3D12> pTextureD3D12 = m_pTexture.Downcast<xiiGALTextureD3D12>();
  if (pTextureD3D12 == nullptr || pTextureD3D12->GetD3D12Texture() == nullptr)
  {
    xiiLog::Error("Failed to initialize D3D12 texture view '{}': backing texture is invalid.", GetDebugName());
    return XII_FAILURE;
  }

  const xiiGALTextureCreationDescription& textureDescription = pTextureD3D12->GetDescription();
  if (textureDescription.m_Usage == xiiGALResourceUsage::Staging)
  {
    xiiLog::Error("Failed to initialize D3D12 texture view '{}': staging textures do not support texture views.", GetDebugName());
    return XII_FAILURE;
  }

  if (!IsTextureViewTypeValidForBindFlags(m_Description.m_ViewType, textureDescription.m_BindFlags))
  {
    xiiLog::Error("Failed to initialize D3D12 texture view '{}': texture bind flags '{}' are incompatible with view type '{}'.", GetDebugName(), textureDescription.m_BindFlags.GetValue(), xiiArgEnum(m_Description.m_ViewType));
    return XII_FAILURE;
  }

  m_ViewMetadata                     = {};
  m_ViewMetadata.m_ViewType          = m_Description.m_ViewType;
  m_ViewMetadata.m_ResourceDimension = m_Description.m_ResourceDimension;
  m_ViewMetadata.m_Format            = m_Description.m_Format;

  switch (m_Description.m_ViewType)
  {
    case xiiGALTextureViewType::ShaderResource:
    case xiiGALTextureViewType::ShadingRate:
      XII_SUCCEED_OR_RETURN(BuildShaderResourceViewDescription(m_Description, textureDescription, m_ViewMetadata));
      break;

    case xiiGALTextureViewType::UnorderedAccess:
      XII_SUCCEED_OR_RETURN(BuildUnorderedAccessViewDescription(m_Description, textureDescription, m_ViewMetadata));
      break;

    case xiiGALTextureViewType::RenderTarget:
      XII_SUCCEED_OR_RETURN(BuildRenderTargetViewDescription(m_Description, textureDescription, m_ViewMetadata));
      break;

    case xiiGALTextureViewType::DepthStencil:
    case xiiGALTextureViewType::ReadOnlyDepthStencil:
      XII_SUCCEED_OR_RETURN(BuildDepthStencilViewDescription(m_Description, textureDescription, m_ViewMetadata));
      break;

    default:
      xiiLog::Error("Failed to initialize D3D12 texture view '{}': unsupported view type '{}'.", GetDebugName(), xiiArgEnum(m_Description.m_ViewType));
      return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiGALTextureViewD3D12::SetDebugNamePlatform(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Resources_Implementation_TextureViewD3D12);
