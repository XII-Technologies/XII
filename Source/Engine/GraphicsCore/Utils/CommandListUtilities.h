#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

// #include <GraphicsFoundation/Resources/Buffer.h>
// #include <GraphicsFoundation/Resources/Sampler.h>
// #include <GraphicsFoundation/Resources/Texture.h>

#include <GraphicsCore/Declarations.h>

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
};
