/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/IO/MemoryStream.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>

#include <GraphicsFoundation/Resources/Sampler.h>

class xiiImage;

/// \brief Use this descriptor in calls to xiiResourceManager::CreateResource<xiiTexture3DResource> to create textures from data in memory.
struct XII_GRAPHICSCORE_DLL xiiTexture3DResourceDescriptor
{
  /// Describes the texture format, etc.
  xiiGALTextureCreationDescription m_DescGAL     = xiiGALTextureUtilities::GetDefaultTexture3DDescription();
  xiiGALSamplerCreationDescription m_SamplerDesc = xiiGALGraphicsUtilities::GetDefaultSamplerDescription();

  /// How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  xiiUInt8 m_uiQualityLevelsDiscardable = 0;

  /// How many additional quality levels can be loaded (typically from file).
  xiiUInt8 m_uiQualityLevelsLoadable = 0;

  /// One memory desc per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not initialize data.
  xiiArrayPtr<xiiGALTextureSubResourceData> m_InitialContent;
};

class XII_GRAPHICSCORE_DLL xiiTexture3DResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTexture3DResource, xiiResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiTexture3DResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiTexture3DResource, xiiTexture3DResourceDescriptor);

public:
  xiiTexture3DResource();

  XII_ALWAYS_INLINE xiiEnum<xiiGALResourceFormat> GetFormat() const { return m_Format; }
  XII_ALWAYS_INLINE xiiUInt32                     GetWidth() const { return m_uiWidth; }
  XII_ALWAYS_INLINE xiiUInt32                     GetHeight() const { return m_uiHeight; }
  XII_ALWAYS_INLINE xiiUInt32                     GetDepth() const { return m_uiDepth; }
  XII_ALWAYS_INLINE xiiEnum<xiiGALResourceDimension> GetType() const { return m_Type; }

  static void FillOutDescriptor(xiiTexture3DResourceDescriptor& ref_td, const xiiImage* pImage, bool bSRGB, xiiUInt32 uiNumMipLevels, xiiUInt32& out_uiMemoryUsed, xiiHybridArray<xiiGALTextureSubResourceData, 32>& ref_initData);

  xiiSharedPtr<xiiGALTexture> GetGALTexture() const { return m_pGALTexture[m_uiLoadedTextures - 1]; }
  xiiSharedPtr<xiiGALSampler> GetGALSampler() const { return m_pSampler; }

protected:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiTexture3DResource(DoUpdate ResourceUpdateThread);

  xiiUInt8                    m_uiLoadedTextures = 0;
  xiiSharedPtr<xiiGALTexture> m_pGALTexture[2];
  xiiUInt32                   m_uiMemoryGPU[2] = {0, 0};

  xiiEnum<xiiGALResourceDimension> m_Type     = xiiGALResourceDimension::Undefined;
  xiiEnum<xiiGALResourceFormat>    m_Format   = xiiGALResourceFormat::Unknown;
  xiiUInt32                        m_uiWidth  = 0;
  xiiUInt32                        m_uiHeight = 0;
  xiiUInt32                        m_uiDepth  = 0;

  xiiSharedPtr<xiiGALSampler> m_pSampler;
};
