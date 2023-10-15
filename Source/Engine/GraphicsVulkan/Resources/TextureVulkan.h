#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

class XII_GRAPHICSVULKAN_DLL xiiGALTextureVulkan final : public xiiGALTexture
{
public:
  virtual xiiGALTextureViewHandle GetDefaultView(xiiEnum<xiiGALTextureViewType> viewType) override;

  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override;

  virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override;

  XII_ALWAYS_INLINE const Diligent::ITexture* GetTexture() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALTextureVulkan(const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTextureVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice, const xiiGALTextureData* pInitialData) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::ITexture> m_pTexture;
};

#include <GraphicsVulkan/Resources/Implementation/TextureVulkan_inl.h>
