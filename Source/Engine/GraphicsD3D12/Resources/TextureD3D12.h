#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

class XII_GRAPHICSD3D12_DLL xiiGALTextureD3D12 final : public xiiGALTexture
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTextureD3D12, xiiGALTexture);

public:
  virtual void SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) override final;

  virtual xiiBitflags<xiiGALResourceStateFlags> GetState() const override final;

  virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override final;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALTextureD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTextureD3D12();

  virtual xiiResult InitPlatform(const xiiGALTextureData* pInitialData) override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};

#include <GraphicsD3D12/Resources/Implementation/TextureD3D12_inl.h>
