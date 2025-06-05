#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Declarations.h>

struct XII_GRAPHICSCORE_DLL xiiDefaultSamplerFlags
{
  using StorageType = xiiUInt32;

  enum Enum
  {
    PointFiltering  = 0,
    LinearFiltering = XII_BIT(0),

    Wrap  = 0,
    Clamp = XII_BIT(1)
  };

  struct Bits
  {
    StorageType LinearFiltering : 1;
    StorageType Clamp : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiDefaultSamplerFlags);

class XII_GRAPHICSCORE_DLL xiiGALCommandListUtilities
{
public:
  static void BindConstantBuffer(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBuffer> pConstantBuffer);

  static void BindBuffer(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBuffer> pBuffer);
  static void BindBufferView(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBufferView> pBufferView);

  static void BindTexture(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTexture> pTexture);
  static void BindTextureView(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTextureView> pTextureView);
  static void BindSampler(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALSampler> pSamplerSate);

  static void BindBufferUAV(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBuffer> pBuffer);
  static void BindBufferViewUAV(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALBufferView> pBufferView);

  static void BindTextureUAV(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTexture> pTexture);
  static void BindTextureViewUAV(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, xiiSharedPtr<xiiGALTextureView> pTextureView);

  static void BindTexture2D(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, const xiiTexture2DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode = xiiResourceAcquireMode::AllowLoadingFallback);
  static void BindTexture3D(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, const xiiTexture3DResourceHandle& hTexture, xiiResourceAcquireMode acquireMode = xiiResourceAcquireMode::AllowLoadingFallback);
  static void BindTextureCube(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiTempHashedString& sSlotName, const xiiTextureCubeResourceHandle& hTexture, xiiResourceAcquireMode acquireMode = xiiResourceAcquireMode::AllowLoadingFallback);
  static void BindMaterial(xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiMaterialResourceHandle& hMaterial);

  static xiiGALSamplerCreationDescription GetDefaultSamplerDescription(xiiBitflags<xiiDefaultSamplerFlags> flags);
};
