#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/IO/MemoryStream.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>

#include <GraphicsFoundation/Descriptors/Descriptors.h>
#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsCore/RenderContext/Implementation/RenderContextStructs.h>

class xiiImage;

using xiiTexture3DResourceHandle = xiiTypedResourceHandle<class xiiTexture3DResource>;

/// \brief Use this descriptor in calls to xiiResourceManager::CreateResource<xiiTexture3DResource> to create textures from data in memory.
struct XII_GRAPHICSCORE_DLL xiiTexture3DResourceDescriptor
{
  /// Describes the texture format, etc.
  xiiGALTextureCreationDescription      m_DescGAL;
  xiiGALSamplerStateCreationDescription m_SamplerDesc;

  /// How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  xiiUInt8 m_uiQualityLevelsDiscardable = 0;

  /// How many additional quality levels can be loaded (typically from file).
  xiiUInt8 m_uiQualityLevelsLoadable = 0;

  /// One memory desc per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not
  /// initialize data.
  xiiArrayPtr<xiiGALSystemMemoryDescription> m_InitialContent;
};

class XII_GRAPHICSCORE_DLL xiiTexture3DResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTexture3DResource, xiiResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiTexture3DResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiTexture3DResource, xiiTexture3DResourceDescriptor);

public:
  xiiTexture3DResource();

  XII_ALWAYS_INLINE xiiGALResourceFormat::Enum GetFormat() const { return m_Format; }
  XII_ALWAYS_INLINE xiiUInt32                  GetWidth() const { return m_uiWidth; }
  XII_ALWAYS_INLINE xiiUInt32                  GetHeight() const { return m_uiHeight; }
  XII_ALWAYS_INLINE xiiUInt32                  GetDepth() const { return m_uiDepth; }
  XII_ALWAYS_INLINE xiiGALTextureType::Enum GetType() const { return m_Type; }

  static void FillOutDescriptor(xiiTexture3DResourceDescriptor& ref_td, const xiiImage* pImage, bool bSRGB, xiiUInt32 uiNumMipLevels, xiiUInt32& out_uiMemoryUsed, xiiHybridArray<xiiGALSystemMemoryDescription, 32>& ref_initData);

  const xiiGALTextureHandle&      GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const xiiGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

protected:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiTexture3DResource(DoUpdate ResourceUpdateThread);

  xiiUInt8            m_uiLoadedTextures = 0;
  xiiGALTextureHandle m_hGALTexture[2];
  xiiUInt32           m_uiMemoryGPU[2] = {0, 0};

  xiiGALTextureType::Enum    m_Type     = xiiGALTextureType::Invalid;
  xiiGALResourceFormat::Enum m_Format   = xiiGALResourceFormat::Invalid;
  xiiUInt32                  m_uiWidth  = 0;
  xiiUInt32                  m_uiHeight = 0;
  xiiUInt32                  m_uiDepth  = 0;

  xiiGALSamplerStateHandle m_hSamplerState;
};
