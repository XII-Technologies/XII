#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>
#include <Texture/Image/Image.h>

using xiiTextureCubeResourceHandle = xiiTypedResourceHandle<class xiiTextureCubeResource>;

/// \brief Use this descriptor in calls to xiiResourceManager::CreateResource<xiiTextureCubeResource> to create textures from data in memory.
struct xiiTextureCubeResourceDescriptor
{
  /// Describes the texture format, etc.
  xiiGALTextureCreationDescription m_TextureDescription = {
    .m_Type               = xiiGALResourceDimension::TextureCube,
    .m_Size               = xiiSizeU32(0, 0),
    .m_uiArraySizeOrDepth = 6U,
    .m_Format             = xiiGALResourceFormat::Unknown,
    .m_uiMipLevels        = 1U,
    .m_uiSampleCount      = xiiGALSampleCount::OneSample,
    .m_BindFlags          = xiiGALBindFlags::ShaderResource,
    .m_Usage              = xiiGALResourceUsage::Immutable,
    .m_CPUAccessFlags     = xiiGALCPUAccessFlag::None,
    .m_MiscFlags          = xiiGALMiscTextureFlags::None,
  };

  xiiGALSamplerCreationDescription m_SamplerDescription = {
    .m_MinFilter          = xiiGALFilterType::Linear,
    .m_MagFilter          = xiiGALFilterType::Linear,
    .m_MipFilter          = xiiGALFilterType::Linear,
    .m_AddressU           = xiiGALTextureAddressMode::Wrap,
    .m_AddressV           = xiiGALTextureAddressMode::Wrap,
    .m_AddressW           = xiiGALTextureAddressMode::Wrap,
    .m_Flags              = xiiGALSamplerFlags::None,
    .m_bUnormalizedCoords = false,
    .m_fMipLODBias        = 0.0f,
    .m_uiMaxAnisotropy    = 4,
    .m_ComparisonFunction = xiiGALComparisonFunction::Never,
    .m_BorderColor        = xiiColor::Black,
    .m_fMinLOD            = -1.0f,
    .m_fMaxLOD            = 4200.0f,
  };

  /// How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  xiiUInt8 m_uiQualityLevelsDiscardable = 0U;

  /// How many additional quality levels can be loaded (typically from file).
  xiiUInt8 m_uiQualityLevelsLoadable = 0U;

  /// One memory desc per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not initialize data.
  xiiArrayPtr<xiiGALTextureSubResourceData> m_InitialContent;
};

class XII_GRAPHICSCORE_DLL xiiTextureCubeResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureCubeResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiTextureCubeResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiTextureCubeResource, xiiTextureCubeResourceDescriptor);

public:
  xiiTextureCubeResource();

  XII_ALWAYS_INLINE xiiEnum<xiiGALResourceFormat> GetFormat() const { return m_Format; }
  XII_ALWAYS_INLINE xiiUInt32                     GetWidthAndHeight() const { return m_uiWidthAndHeight; }

  xiiSharedPtr<xiiGALTexture> GetGALTexture() const { return m_pGALTexture[m_uiLoadedTextures - 1]; }
  xiiSharedPtr<xiiGALSampler> GetGALSampler() const { return m_pSampler; }

protected:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiUInt8                    m_uiLoadedTextures;
  xiiSharedPtr<xiiGALTexture> m_pGALTexture[2];
  xiiUInt32                   m_uiMemoryGPU[2];

  xiiEnum<xiiGALResourceFormat> m_Format;
  xiiUInt32                     m_uiWidthAndHeight;

  xiiSharedPtr<xiiGALSampler> m_pSampler;
};
