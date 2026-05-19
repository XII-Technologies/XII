/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Texture.h>

struct ID3D12Resource;

class XII_GRAPHICSD3D12_DLL xiiGALTextureD3D12 final : public xiiGALTexture
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTextureD3D12, xiiGALTexture);

public:
  [[nodiscard]] XII_ALWAYS_INLINE ID3D12Resource*    GetD3D12Texture() const { return m_pD3D12Texture; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiD3D12Allocation GetAllocationDescription() const { return m_TextureAllocation; }
  [[nodiscard]] XII_ALWAYS_INLINE bool               IsStagingTexture() const { return m_Description.m_Usage == xiiGALResourceUsage::Staging; }

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
  ID3D12Resource*    m_pD3D12Texture      = nullptr;
  xiiD3D12Allocation m_TextureAllocation  = nullptr;
  bool               m_bHostVisibleUpload = false;
  bool               m_bReadbackTexture   = false;

  xiiGALSparseTextureProperties m_SparseTextureProperties;
};
