#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Math/Rect.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <Texture/Utils/TextureAtlasDesc.h>

using xiiDecalAtlasResourceHandle = xiiTypedResourceHandle<class xiiDecalAtlasResource>;
using xiiTexture2DResourceHandle  = xiiTypedResourceHandle<class xiiTexture2DResource>;

class xiiImage;

struct xiiDecalAtlasResourceDescriptor
{
};

class XII_RENDERERCORE_DLL xiiDecalAtlasResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalAtlasResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiDecalAtlasResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiDecalAtlasResource, xiiDecalAtlasResourceDescriptor);

public:
  xiiDecalAtlasResource();

  /// \brief Returns the one global decal atlas resource
  static xiiDecalAtlasResourceHandle GetDecalAtlasResource();

  const xiiTexture2DResourceHandle& GetBaseColorTexture() const { return m_hBaseColor; }
  const xiiTexture2DResourceHandle& GetNormalTexture() const { return m_hNormal; }
  const xiiTexture2DResourceHandle& GetORMTexture() const { return m_hORM; }
  const xiiVec2U32&                 GetBaseColorTextureSize() const { return m_vBaseColorSize; }
  const xiiVec2U32&                 GetNormalTextureSize() const { return m_vNormalSize; }
  const xiiVec2U32&                 GetORMTextureSize() const { return m_vORMSize; }
  const xiiTextureAtlasRuntimeDesc& GetAtlas() const { return m_Atlas; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                ReportResourceIsMissing() override;

  void ReadDecalInfo(xiiStreamReader* Stream);

  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void CreateLayerTexture(const xiiImage& img, bool bSRGB, xiiTexture2DResourceHandle& out_hTexture);

  xiiTextureAtlasRuntimeDesc m_Atlas;
  static xiiUInt32           s_uiDecalAtlasResources;
  xiiTexture2DResourceHandle m_hBaseColor;
  xiiTexture2DResourceHandle m_hNormal;
  xiiTexture2DResourceHandle m_hORM;
  xiiVec2U32                 m_vBaseColorSize;
  xiiVec2U32                 m_vNormalSize;
  xiiVec2U32                 m_vORMSize;
};
