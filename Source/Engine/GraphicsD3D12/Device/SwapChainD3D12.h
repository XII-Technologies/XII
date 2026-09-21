/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Device/SwapChain.h>

struct IDXGISwapChain3;

/// This describes how an image is stretched to fit a given monitor's resolution.
/// \sa <a href = "https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/bb173066(v=vs.85)">DXGI_MODE_SCALING enumeration on MSDN</a>,
struct XII_GRAPHICSD3D12_DLL xiiGALScalingModeD3D12
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unspecified = 0U, ///< Unspecified scaling.
    Centered,         ///< Specifies no scaling. The image is centered on the display. This flag is typically used for a fixed-dot-pitch display (such as an LED display).
    Stretched,        ///< Specifies a stretched scaling.

    ENUM_COUNT,

    Default = Unspecified
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSD3D12_DLL, xiiGALScalingModeD3D12);

/// This describes the method the raster uses to create an image on a surface.
/// \sa <a href = "https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/bb173067(v=vs.85)">DXGI_MODE_SCANLINE_ORDER enumeration on MSDN</a>,
struct XII_GRAPHICSD3D12_DLL xiiGALScanLineOrderD3D12
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Unspecified = 0U, ///< Unspecified scanline order.
    Progressive,      ///< The image is created from the first scanline to the last without skipping any.
    UpperFieldFirst,  ///< The image is created beginning with the upper field.
    LowerFieldFirst,  ///< The image is created beginning with the lower field.

    ENUM_COUNT,

    Default = Unspecified
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSD3D12_DLL, xiiGALScanLineOrderD3D12);

/// This describes the display mode attributes.
struct XII_GRAPHICSD3D12_DLL xiiGALDisplayModeDescriptionD3D12 : public xiiHashableStruct<xiiGALDisplayModeDescriptionD3D12>
{
  XII_DECLARE_POD_TYPE();

  xiiSizeU32                        m_Resolution               = xiiSizeU32(0U, 0U);                    ///< Display resolution.
  xiiEnum<xiiGALResourceFormat>     m_ResourceFormat           = xiiGALResourceFormat::Unknown;         ///< Display format.
  xiiUInt32                         m_uiRefreshRateNumerator   = 0U;                                    ///< Refresh rate numerator.
  xiiUInt32                         m_uiRefreshRateDenominator = 0U;                                    ///< Refresh rate denominator.
  xiiEnum<xiiGALScalingModeD3D12>   m_ScalingMode              = xiiGALScalingModeD3D12::Unspecified;   ///< The scaling mode.
  xiiEnum<xiiGALScanLineOrderD3D12> m_ScanLineOrder            = xiiGALScanLineOrderD3D12::Unspecified; ///< The scanline drawing mode.
};

/// This describes the full screen mode description.
struct XII_GRAPHICSD3D12_DLL xiiGALFullScreenModeDescriptionD3D12 : public xiiHashableStruct<xiiGALFullScreenModeDescriptionD3D12>
{
  XII_DECLARE_POD_TYPE();

  bool                              m_bIsFullScreen            = false;                                 ///< Specifies whether the swap chain is in full screen mode.
  xiiUInt32                         m_uiRefreshRateNumerator   = 0U;                                    ///< Refresh rate numerator.
  xiiUInt32                         m_uiRefreshRateDenominator = 0U;                                    ///< Refresh rate denominator.
  xiiEnum<xiiGALScalingModeD3D12>   m_ScalingMode              = xiiGALScalingModeD3D12::Unspecified;   ///< The scaling mode.
  xiiEnum<xiiGALScanLineOrderD3D12> m_ScanLineOrder            = xiiGALScanLineOrderD3D12::Unspecified; ///< The scanline drawing mode.
};

class XII_GRAPHICSD3D12_DLL xiiGALSwapChainD3D12 final : public xiiGALSwapChain
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALSwapChainD3D12, xiiGALSwapChain);

public:
  virtual void Present() override final;

  virtual xiiResult Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform = xiiGALSurfaceTransform::Optimal) override final;

  void SetFullScreenMode(const xiiGALDisplayModeDescriptionD3D12& displayMode);

  void SetWindowedMode();

  void SetMaximumFrameLatency(xiiUInt32 uiMaxLatency);

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;
  friend class xiiGALTexture;

  xiiGALSwapChainD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALSwapChainCreationDescription& creationDescription);

  virtual ~xiiGALSwapChainD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

  xiiResult CreateDXGISwapChain();

  xiiResult UpdateSwapChain(bool bCreateNew);

  xiiResult CreateBackBufferInternal();

  void WaitForFrame();

protected:
  IDXGISwapChain3* m_pDXGISwapChain3 = nullptr;

  xiiUInt32                                       m_uiCurrentBackBufferIndex = 0U;
  xiiHybridArray<xiiSharedPtr<xiiGALTexture>, 2U> m_SwapChainTextures;

  xiiGALFullScreenModeDescriptionD3D12 m_FullScreenMode;

  HANDLE m_FrameLatencyWaitableObject = NULL;

  xiiUInt32 m_uiMaximumFrameLatency = 1U;
};
