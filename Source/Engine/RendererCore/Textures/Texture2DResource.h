#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class xiiImage;

using xiiTexture2DResourceHandle = xiiTypedResourceHandle<class xiiTexture2DResource>;

/// \brief Use this descriptor in calls to xiiResourceManager::CreateResource<xiiTexture2DResource> to create textures from data in memory.
struct XII_RENDERERCORE_DLL xiiTexture2DResourceDescriptor
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

class XII_RENDERERCORE_DLL xiiTexture2DResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTexture2DResource, xiiResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiTexture2DResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiTexture2DResource, xiiTexture2DResourceDescriptor);

public:
  xiiTexture2DResource();

  XII_ALWAYS_INLINE xiiGALResourceFormat::Enum GetFormat() const { return m_Format; }
  XII_ALWAYS_INLINE xiiUInt32                  GetWidth() const { return m_uiWidth; }
  XII_ALWAYS_INLINE xiiUInt32                  GetHeight() const { return m_uiHeight; }
  XII_ALWAYS_INLINE xiiGALTextureType::Enum GetType() const { return m_Type; }

  static void FillOutDescriptor(xiiTexture2DResourceDescriptor& ref_td, const xiiImage* pImage, bool bSRGB, xiiUInt32 uiNumMipLevels, xiiUInt32& out_uiMemoryUsed, xiiHybridArray<xiiGALSystemMemoryDescription, 32>& ref_initData);

  const xiiGALTextureHandle&      GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const xiiGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

protected:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiTexture2DResource(DoUpdate ResourceUpdateThread);

  xiiUInt8            m_uiLoadedTextures = 0;
  xiiGALTextureHandle m_hGALTexture[2];
  xiiUInt32           m_uiMemoryGPU[2] = {0, 0};

  xiiGALTextureType::Enum    m_Type     = xiiGALTextureType::Invalid;
  xiiGALResourceFormat::Enum m_Format   = xiiGALResourceFormat::Invalid;
  xiiUInt32                  m_uiWidth  = 0;
  xiiUInt32                  m_uiHeight = 0;

  xiiGALSamplerStateHandle m_hSamplerState;
};

//////////////////////////////////////////////////////////////////////////

using xiiRenderToTexture2DResourceHandle = xiiTypedResourceHandle<class xiiRenderToTexture2DResource>;

struct XII_RENDERERCORE_DLL xiiRenderToTexture2DResourceDescriptor
{
  xiiUInt32                                  m_uiWidth  = 0;
  xiiUInt32                                  m_uiHeight = 0;
  xiiEnum<xiiGALMSAASampleCount>             m_SampleCount;
  xiiEnum<xiiGALResourceFormat>              m_Format;
  xiiGALSamplerStateCreationDescription      m_SamplerDesc;
  xiiArrayPtr<xiiGALSystemMemoryDescription> m_InitialContent;
};

class XII_RENDERERCORE_DLL xiiRenderToTexture2DResource : public xiiTexture2DResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderToTexture2DResource, xiiTexture2DResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiRenderToTexture2DResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiRenderToTexture2DResource, xiiRenderToTexture2DResourceDescriptor);

public:
  xiiGALRenderTargetViewHandle          GetRenderTargetView() const;
  void                                  AddRenderView(xiiViewHandle hView);
  void                                  RemoveRenderView(xiiViewHandle hView);
  const xiiDynamicArray<xiiViewHandle>& GetAllRenderViews() const;

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

protected:
  // other views that use this texture as their target
  xiiDynamicArray<xiiViewHandle> m_RenderViews;
};
