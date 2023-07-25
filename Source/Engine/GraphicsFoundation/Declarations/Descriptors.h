#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Size.h>

#include <GraphicsFoundation/Declarations/Constants.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief This describes the optimized depth-stencil clear value.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceFeatures : public xiiHashableStruct<xiiGALDeviceFeatures>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALDeviceFeatureState> m_SeparablePrograms;
  xiiEnum<xiiGALDeviceFeatureState> m_ShaderResourceQueries;
  xiiEnum<xiiGALDeviceFeatureState> m_WireframeFill;
  xiiEnum<xiiGALDeviceFeatureState> m_MultithreadedResourceCreation;
  xiiEnum<xiiGALDeviceFeatureState> m_ComputeShaders;
};

/// \brief This describes the optimized depth-stencil clear value.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDepthStencilClearValue : public xiiHashableStruct<xiiGALDepthStencilClearValue>
{
  XII_DECLARE_POD_TYPE();

  float    m_fDepth    = 1.0f; ///< Depth clear value.
  xiiUInt8 m_uiStencil = 0;    ///< Stencil clear value.
};

/// \brief This describes the optimized color clear value.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALOptimizedClearValue : public xiiHashableStruct<xiiGALOptimizedClearValue>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALTextureFormat> m_TextureFormat; ///< Texture format.
  xiiColor                     m_ClearColor;    ///< Render target clear value.
  xiiGALDepthStencilClearValue m_DepthStencil;  ///< Depth stencil clear value.
};

/// \brief This describes the display mode attributes.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALOptimizedClearValue : public xiiHashableStruct<xiiGALOptimizedClearValue>
{
  XII_DECLARE_POD_TYPE();

  xiiSizeU32                   m_Resolution = xiiSizeU32(0U, 0U); ///< Display resolution.
  xiiEnum<xiiGALTextureFormat> m_TextureFormat;                   ///< Display format.
  xiiUInt32                    m_uiRefreshRateNumerator   = 0U;   ///< Refresh rate numerator.
  xiiUInt32                    m_uiRefreshRateDenominator = 0U;   ///< Refresh rate denominator.
  xiiEnum<xiiGALScalingMode>   m_ScalingMode;                     ///< The scaling mode.
  xiiEnum<xiiGALScanLineOrder> m_ScanLineOrder;                   ///< The scanline drawing mode.
};

/// \brief This describes the swap chain creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSwapchainCreationDescription : public xiiHashableStruct<xiiGALSwapchainCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiSizeU32                         m_Resolution        = xiiSizeU32(0U, 0U);                        ///< Swap chain resolution.
  xiiEnum<xiiGALTextureFormat>       m_ColorBufferFormat = xiiGALTextureFormat::RGBA8UNormalizedSRGB; ///< Back buffer format.
  xiiEnum<xiiGALTextureFormat>       m_DepthBufferFormat = xiiGALTextureFormat::D32Float;             ///< Depth buffer format. Use Unknown format to create the swapchain without a depth buffer.
  xiiEnum<xiiGALSwapChainUsageFlags> m_Usage             = xiiGALSwapChainUsageFlags::RenderTarget;   ///< Swap chain usage flags.
  xiiEnum<xiiGALSurfaceTransform>    m_PreTransform      = xiiGALSurfaceTransform::Optimal;           ///< The transform, relative to the presentation engine's natural orientation which is applied to the image prior to presentation.
                                                                                                      ///<
                                                                                                      ///< \note When xiiGALSurfaceTransform::Optimal is used, the engine will select the most optimal surface transformation.
                                                                                                      ///< An application may request a specific transform and the engine will try to use that. If the transform is not available, the engine will
                                                                                                      ///< select the most optimal transform. After the swapchain has been created, this member will contain the actual transform selected by the engine.
  xiiUInt32 m_uiBufferCount         = 2U;                                                             ///< The number of buffers in the swap chain.
  float     m_fDepthStencilValue    = 1.0f;                                                           ///< Default depth value, which is used as the optimized depth clear value in D3D12.
  xiiUInt8  m_uiDefaultStencilValue = 0U;                                                             ///< Default stencil value, which is used as the optimized clear value in D3D12.
  bool      m_bIsPrimary            = true;                                                           ///< This indicates if this swap chain is a primary swap chain.
};

/// \brief This describes the full screen mode description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALFullScreenModeDescription : public xiiHashableStruct<xiiGALFullScreenModeDescription>
{
  XII_DECLARE_POD_TYPE();

  bool                         m_bIsFullScreen            = false; ///< Specifies whether the swapchain is in full screen mode.
  xiiUInt32                    m_uiRefreshRateNumerator   = 0U;    ///< Refresh rate numerator.
  xiiUInt32                    m_uiRefreshRateDenominator = 0U;    ///< Refresh rate denominator.
  xiiEnum<xiiGALScalingMode>   m_ScalingMode;                      ///< The scaling mode.
  xiiEnum<xiiGALScanLineOrder> m_ScanLineOrder;                    ///< The scanline drawing mode.
};

#include <GraphicsFoundation/Declarations/Implementation/Descriptors_inl.h>
