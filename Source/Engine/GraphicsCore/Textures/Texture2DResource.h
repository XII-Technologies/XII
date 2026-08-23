/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/IO/MemoryStream.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>

#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

class xiiImage;

using xiiTexture2DResourceHandle = xiiTypedResourceHandle<class xiiTexture2DResource>;

/// \brief Use this descriptor in calls to xiiResourceManager::CreateResource<xiiTexture2DResource> to create textures from data in memory.
struct XII_GRAPHICSCORE_DLL xiiTexture2DResourceDescriptor
{
  xiiGALTextureCreationDescription          m_TextureDescription         = xiiGALTextureUtilities::GetDefaultTexture2DDescription(); ///< Texture creation description.
  xiiGALSamplerCreationDescription          m_SamplerDescription         = xiiGALGraphicsUtilities::GetDefaultSamplerDescription();  ///< Sampler creation description.
  xiiUInt8                                  m_uiQualityLevelsDiscardable = 0;                                                        ///< How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  xiiUInt8                                  m_uiQualityLevelsLoadable    = 0;                                                        ///< How many additional quality levels can be loaded (typically from file).
  xiiArrayPtr<xiiGALTextureSubResourceData> m_InitialContent;                                                                        ///< One texture subresource data per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not initialize data.
};

class XII_GRAPHICSCORE_DLL xiiTexture2DResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTexture2DResource, xiiResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiTexture2DResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiTexture2DResource, xiiTexture2DResourceDescriptor);

public:
  xiiTexture2DResource();

  XII_ALWAYS_INLINE xiiEnum<xiiGALResourceFormat> GetFormat() const { return m_Format; }
  XII_ALWAYS_INLINE xiiUInt32                     GetWidth() const { return m_uiWidth; }
  XII_ALWAYS_INLINE xiiUInt32                     GetHeight() const { return m_uiHeight; }
  XII_ALWAYS_INLINE xiiEnum<xiiGALResourceDimension> GetType() const { return m_Type; }

  static void FillOutDescriptor(xiiTexture2DResourceDescriptor& ref_td, const xiiImage* pImage, xiiUInt32 uiNumMipLevels, xiiUInt32& out_uiMemoryUsed, xiiHybridArray<xiiGALTextureSubResourceData, 32>& ref_initData);

  xiiSharedPtr<xiiGALTexture> GetGALTexture() const { return m_pGALTexture[m_uiLoadedTextures - 1]; }
  xiiSharedPtr<xiiGALSampler> GetGALSampler() const { return m_pSampler; }

protected:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  virtual void                       UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiTexture2DResource(DoUpdate ResourceUpdateThread);

  xiiUInt8                    m_uiLoadedTextures = 0;
  xiiSharedPtr<xiiGALTexture> m_pGALTexture[2];
  xiiUInt32                   m_uiMemoryGPU[2] = {0, 0};

  xiiEnum<xiiGALResourceDimension> m_Type     = xiiGALResourceDimension::Undefined;
  xiiEnum<xiiGALResourceFormat>    m_Format   = xiiGALResourceFormat::Unknown;
  xiiUInt32                        m_uiWidth  = 0;
  xiiUInt32                        m_uiHeight = 0;

  xiiSharedPtr<xiiGALSampler> m_pSampler;
};
