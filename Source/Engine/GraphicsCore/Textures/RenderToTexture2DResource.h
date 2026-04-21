#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

using xiiRenderToTexture2DResourceHandle = xiiTypedResourceHandle<class xiiRenderToTexture2DResource>;

struct XII_GRAPHICSCORE_DLL xiiRenderToTexture2DResourceDescriptor
{
  xiiUInt32                                 m_uiWidth  = 0;
  xiiUInt32                                 m_uiHeight = 0;
  xiiEnum<xiiGALResourceFormat>             m_Format   = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  xiiEnum<xiiGALSampleCount>                m_SampleCount;
  xiiGALSamplerCreationDescription          m_SamplerDesc = xiiGALGraphicsUtilities::GetDefaultSamplerDescription();
  xiiArrayPtr<xiiGALTextureSubResourceData> m_InitialContent;
};

class XII_GRAPHICSCORE_DLL xiiRenderToTexture2DResource : public xiiTexture2DResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderToTexture2DResource, xiiTexture2DResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiRenderToTexture2DResource);

  XII_RESOURCE_DECLARE_CREATEABLE(xiiRenderToTexture2DResource, xiiRenderToTexture2DResourceDescriptor);

public:
  /// \brief Creates a render target view for this texture. This is used when this texture is used as a render target, and allows to bind the texture to the pipeline in that case.
  xiiSharedPtr<xiiGALTextureView> GetRenderTargetView() const;

  /// \brief Creates a shader resource view for this texture. This is used when this texture is used as a regular texture, and allows to bind the texture to the pipeline in that case.
  void AddRenderView(xiiViewHandle hView);

  /// \brief Removes a render target view for this texture. This should be called when a view that uses this texture as a render target gets destroyed, to remove the reference to the view from this resource.
  void RemoveRenderView(xiiViewHandle hView);

  /// \brief Returns all views that use this texture as their render target. This is used to invalidate those views when this resource gets unloaded or destroyed.
  const xiiDynamicArray<xiiViewHandle>& GetAllRenderViews() const;

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

protected:
  // Other views that use this texture as their target.
  xiiDynamicArray<xiiViewHandle> m_RenderViews;
};
