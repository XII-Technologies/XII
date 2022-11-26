
#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

class xiiGALRenderTargetViewVulkan : public xiiGALRenderTargetView
{
public:
  vk::ImageView             GetImageView() const;
  bool                      IsFullRange() const;
  vk::ImageSubresourceRange GetRange() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALRenderTargetViewVulkan(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& Description);
  virtual ~xiiGALRenderTargetViewVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

  vk::ImageView             m_imageView;
  bool                      m_bfullRange = false;
  vk::ImageSubresourceRange m_range;
};

#include <RendererVulkan/Resources/Implementation/RenderTargetViewVulkan_inl.h>
