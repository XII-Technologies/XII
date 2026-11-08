/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Time/Time.h>
#include <GraphicsCore/Lighting/EnvironmentMap.h>
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
  bool                                      m_bStreamingRefinement       = false;                                                    ///< A resident image remains available if creation is deferred.
  xiiArrayPtr<xiiGALTextureSubResourceData> m_InitialContent;                                                                        ///< One texture subresource data per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not initialize data.
};

class XII_GRAPHICSCORE_DLL xiiTextureCubeResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureCubeResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiTextureCubeResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiTextureCubeResource, xiiTextureCubeResourceDescriptor);

public:
  xiiTextureCubeResource();

  XII_ALWAYS_INLINE xiiEnum<xiiGALResourceFormat> GetFormat() const
  {
    XII_LOCK(m_TextureMutex);
    return m_Format;
  }
  XII_ALWAYS_INLINE xiiUInt32 GetWidthAndHeight() const
  {
    XII_LOCK(m_TextureMutex);
    return m_uiWidthAndHeight;
  }

  xiiSharedPtr<xiiGALTexture> GetGALTexture() const
  {
    XII_LOCK(m_TextureMutex);
    return m_pGALTexture;
  }
  xiiSharedPtr<xiiGALSampler> GetGALSampler() const
  {
    XII_LOCK(m_TextureMutex);
    return m_pSampler;
  }
  /// Lazily submits convolution for the current resident source. Reloaded quality
  /// levels receive distinct immutable results; retained consumers own older images.
  /// First acquisition requires the thread owning an active xiiRenderGraphFrame.
  xiiStatus GetEnvironmentMap(xiiSharedPtr<const xiiEnvironmentMap>& out_map);

protected:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  [[nodiscard]] virtual bool         CanLoadAdditionalQualityLevel() const override;
  virtual void                       UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  mutable xiiMutex            m_TextureMutex;
  xiiTime                     m_NextRefinementAttempt;
  xiiSharedPtr<xiiGALTexture> m_pGALTexture;

  xiiEnum<xiiGALResourceFormat> m_Format;
  xiiUInt32                     m_uiWidthAndHeight;

  xiiSharedPtr<xiiGALSampler>           m_pSampler;
  xiiSharedPtr<const xiiEnvironmentMap> m_pEnvironmentMap;
};
