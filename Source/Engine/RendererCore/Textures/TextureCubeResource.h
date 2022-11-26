#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <Texture/Image/Image.h>

using xiiTextureCubeResourceHandle = xiiTypedResourceHandle<class xiiTextureCubeResource>;

/// \brief Use this descriptor in calls to xiiResourceManager::CreateResource<xiiTextureCubeResource> to create textures from data in memory.
struct xiiTextureCubeResourceDescriptor
{
  xiiTextureCubeResourceDescriptor()
  {
    m_uiQualityLevelsDiscardable = 0;
    m_uiQualityLevelsLoadable    = 0;
  }

  /// Describes the texture format, etc.
  xiiGALTextureCreationDescription      m_DescGAL;
  xiiGALSamplerStateCreationDescription m_SamplerDesc;

  /// How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  xiiUInt8 m_uiQualityLevelsDiscardable;

  /// How many additional quality levels can be loaded (typically from file).
  xiiUInt8 m_uiQualityLevelsLoadable;

  /// One memory desc per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not
  /// initialize data.
  xiiArrayPtr<xiiGALSystemMemoryDescription> m_InitialContent;
};

class XII_RENDERERCORE_DLL xiiTextureCubeResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureCubeResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiTextureCubeResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiTextureCubeResource, xiiTextureCubeResourceDescriptor);

public:
  xiiTextureCubeResource();

  XII_ALWAYS_INLINE xiiGALResourceFormat::Enum GetFormat() const { return m_Format; }
  XII_ALWAYS_INLINE xiiUInt32                  GetWidthAndHeight() const { return m_uiWidthAndHeight; }

  const xiiGALTextureHandle&      GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const xiiGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

protected:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiUInt8            m_uiLoadedTextures;
  xiiGALTextureHandle m_hGALTexture[2];
  xiiUInt32           m_uiMemoryGPU[2];

  xiiGALResourceFormat::Enum m_Format;
  xiiUInt32                  m_uiWidthAndHeight;

  xiiGALSamplerStateHandle m_hSamplerState;
};
