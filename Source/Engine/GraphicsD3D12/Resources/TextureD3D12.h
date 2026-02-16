#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

class XII_GRAPHICSD3D12_DLL xiiGALTextureD3D12 final : public xiiGALTexture
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTextureD3D12, xiiGALTexture);

public:
  [[nodiscard]] XII_ALWAYS_INLINE bool IsNativeObjectWrapper() const { return m_Description.m_pExistingNativeObject != nullptr; }

  virtual const xiiGALSparseTextureProperties& GetSparseProperties() const override final;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALTextureD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALTextureCreationDescription& creationDescription);

  virtual ~xiiGALTextureD3D12();

  virtual xiiResult InitPlatform(const xiiGALTextureData* pInitialData, xiiBitflags<xiiGALExternalMemoryKind> externalMemoryKind) override final;

  virtual xiiInternal::NewInstance<xiiGALTextureView> CreateViewPlatform(const xiiGALTextureViewCreationDescription& description) override;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  xiiGALSparseTextureProperties m_SparseTextureProperties;
};
