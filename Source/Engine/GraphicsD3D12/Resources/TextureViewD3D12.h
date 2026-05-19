/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/TextureView.h>

class XII_GRAPHICSD3D12_DLL xiiGALTextureViewD3D12 final : public xiiGALTextureView
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTextureViewD3D12, xiiGALTextureView);

public:
  struct ViewMetadata
  {
    xiiEnum<xiiGALTextureViewType>  m_ViewType          = xiiGALTextureViewType::Undefined;
    xiiEnum<xiiGALResourceDimension> m_ResourceDimension = xiiGALResourceDimension::Undefined;
    xiiEnum<xiiGALResourceFormat>   m_Format            = xiiGALResourceFormat::Unknown;

    D3D12_SHADER_RESOURCE_VIEW_DESC    m_ShaderResourceView = {};
    D3D12_UNORDERED_ACCESS_VIEW_DESC   m_UnorderedAccessView = {};
    D3D12_RENDER_TARGET_VIEW_DESC      m_RenderTargetView = {};
    D3D12_DEPTH_STENCIL_VIEW_DESC      m_DepthStencilView = {};

    bool m_bHasShaderResourceView   = false;
    bool m_bHasUnorderedAccessView  = false;
    bool m_bHasRenderTargetView     = false;
    bool m_bHasDepthStencilView     = false;
  };

  [[nodiscard]] XII_ALWAYS_INLINE const ViewMetadata& GetViewMetadata() const { return m_ViewMetadata; }

protected:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceD3D12;
  friend class xiiGALTextureD3D12;

  xiiGALTextureViewD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, xiiSharedPtr<xiiGALTexture> pTexture, const xiiGALTextureViewCreationDescription& creationDescription);

  virtual ~xiiGALTextureViewD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  ViewMetadata m_ViewMetadata;
};
