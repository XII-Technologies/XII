#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Textures/Texture2DResource.h>

using xiiRenderToTexture2DResourceHandle = xiiTypedResourceHandle<class xiiRenderToTexture2DResource>;

struct XII_GRAPHICSCORE_DLL xiiRenderToTexture2DResourceDescriptor
{
  xiiUInt32                                 m_uiWidth  = 0;
  xiiUInt32                                 m_uiHeight = 0;
  xiiEnum<xiiGALTextureFormat>              m_Format   = xiiGALTextureFormat::RGBA8UNormalizedSRGB;
  xiiEnum<xiiGALMSAASampleCount>            m_SampleCount;
  xiiGALSamplerCreationDescription          m_SamplerDesc;
  xiiArrayPtr<xiiGALTextureSubResourceData> m_InitialContent;
};

class XII_GRAPHICSCORE_DLL xiiRenderToTexture2DResource : public xiiTexture2DResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderToTexture2DResource, xiiTexture2DResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiRenderToTexture2DResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiRenderToTexture2DResource, xiiRenderToTexture2DResourceDescriptor);

public:
  xiiGALTextureViewHandle               GetRenderTargetView() const;
  void                                  AddRenderView(xiiViewHandle hView);
  void                                  RemoveRenderView(xiiViewHandle hView);
  const xiiDynamicArray<xiiViewHandle>& GetAllRenderViews() const;

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

protected:
  // Other views that use this texture as their target
  xiiDynamicArray<xiiViewHandle> m_RenderViews;
};
