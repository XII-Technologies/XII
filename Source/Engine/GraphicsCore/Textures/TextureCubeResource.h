/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

/// Use this descriptor in calls to xiiResourceManager::CreateResource<xiiTextureCubeResource> to create textures from data in memory.
struct XII_GRAPHICSCORE_DLL xiiTextureCubeResourceDescriptor
{
  xiiGALTextureCreationDescription          m_TextureDescription         = xiiGALTextureUtilities::GetDefaultTexture2DDescription(); ///< Texture creation description.
  xiiGALSamplerCreationDescription          m_SamplerDescription         = xiiGALGraphicsUtilities::GetDefaultSamplerDescription();  ///< Sampler creation description.
  xiiUInt8                                  m_uiQualityLevelsDiscardable = 0;                                                        ///< How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  xiiUInt8                                  m_uiQualityLevelsLoadable    = 0;                                                        ///< How many additional quality levels can be loaded (typically from file).
  xiiArrayPtr<xiiGALTextureSubResourceData> m_InitialContent;                                                                        ///< One texture subresource data per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not initialize data.
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
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  virtual void                       UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiUInt8                    m_uiLoadedTextures;
  xiiSharedPtr<xiiGALTexture> m_pGALTexture[2];
  xiiUInt32                   m_uiMemoryGPU[2];

  xiiEnum<xiiGALResourceFormat> m_Format;
  xiiUInt32                     m_uiWidthAndHeight;

  xiiSharedPtr<xiiGALSampler> m_pSampler;
};
