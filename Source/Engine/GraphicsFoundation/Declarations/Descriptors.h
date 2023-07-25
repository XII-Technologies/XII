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

  xiiEnum<xiiGALDeviceFeatureState> m_SeparablePrograms;                 ///< Indicates if the device supports separable shader programs.
  xiiEnum<xiiGALDeviceFeatureState> m_ShaderResourceQueries;             ///< Indicates if the device supports resource queries from shader objects.
  xiiEnum<xiiGALDeviceFeatureState> m_WireframeFill;                     ///< Indicates if the device supports wireframe fill mode.
  xiiEnum<xiiGALDeviceFeatureState> m_MultithreadedResourceCreation;     ///< Indicates if the device supports multithreaded resource creation.
  xiiEnum<xiiGALDeviceFeatureState> m_ComputeShaders;                    ///< Indicates if the device supports compute shaders.
  xiiEnum<xiiGALDeviceFeatureState> m_GeometryShaders;                   ///< Indicates if the device supports geometry shaders.
  xiiEnum<xiiGALDeviceFeatureState> m_Tessellation;                      ///< Indicates if the device supports tessellation.
  xiiEnum<xiiGALDeviceFeatureState> m_MeshShaders;                       ///< Indicates if the device supports mesh and amplification shaders.
  xiiEnum<xiiGALDeviceFeatureState> m_RayTracing;                        ///< Indicates if the device supports ray tracing.
  xiiEnum<xiiGALDeviceFeatureState> m_BindlessResources;                 ///< Indicates if the device supports bindless resources.
  xiiEnum<xiiGALDeviceFeatureState> m_OcclusionQueries;                  ///< Indicates if the device supports occlusion queries.
  xiiEnum<xiiGALDeviceFeatureState> m_BinaryOcclusionQueries;            ///< Indicates if the device supports binary occlusion queries.
  xiiEnum<xiiGALDeviceFeatureState> m_TimestampQueries;                  ///< Indicates if the device supports timestamp queries.
  xiiEnum<xiiGALDeviceFeatureState> m_PipelineStateQueries;              ///< Indicates if the device supports pipeline statistics queries.
  xiiEnum<xiiGALDeviceFeatureState> m_DurationQueries;                   ///< Indicates if the device supports duration queries.
  xiiEnum<xiiGALDeviceFeatureState> m_DepthBiasClamp;                    ///< Indicates if the device supports depth bias clamp.
  xiiEnum<xiiGALDeviceFeatureState> m_DepthClamp;                        ///< Indicates if the device supports depth clamp.
  xiiEnum<xiiGALDeviceFeatureState> m_IndependentBlend;                  ///< Indicates if the device supports independent blend.
  xiiEnum<xiiGALDeviceFeatureState> m_DualSourceBlend;                   ///< Indicates if the device supports dual-source blend.
  xiiEnum<xiiGALDeviceFeatureState> m_MultiViewport;                     ///< Indicates if the device supports multiviewport.
  xiiEnum<xiiGALDeviceFeatureState> m_TextureCompressionBC;              ///< Indicates if the device supports all BC-compressed formats.
  xiiEnum<xiiGALDeviceFeatureState> m_VertexPipelineUAVWritesAndAtomics; ///< Indicates if the device supports writes to UAVs as well as atomic operations in vertex, tessellation, and geometry shader stages.
  xiiEnum<xiiGALDeviceFeatureState> m_PixelUAVWritesAndAtomics;          ///< Indicates if the device supports writes to UAVs as well as atomic operations in pixel shader stage.
  xiiEnum<xiiGALDeviceFeatureState> m_TextureUAVExtendedFormats;         ///< Indicates if the device supports all the extended UAV texture formats available in the shader code.
  xiiEnum<xiiGALDeviceFeatureState> m_ShaderFloat16;                     ///< Indicates if the device supports native 16-bit float operations. Note that there are separate features that indicate if device supports loading 16-bit floats from buffers and passing them between shader stages. See https://therealmjp.github.io/posts/shader-fp16/ more info.
  xiiEnum<xiiGALDeviceFeatureState> m_ResourceBuffer16BitAccess;         ///< Indicates if the device supports reading and writing 16-bit floats and integers from buffers bound as shader resource or unordered access views.
  xiiEnum<xiiGALDeviceFeatureState> m_UniformBuffer16BitAccess;          ///< Indicates if the device supports reading 16-bit floats and integers from uniform buffers.
  xiiEnum<xiiGALDeviceFeatureState> m_ShaderInputOutput16;               ///< Indicates if the device supports using 16-bit floats and integers as input/output of a shader entry point.
  xiiEnum<xiiGALDeviceFeatureState> m_ShaderInt8;                        ///< Indicates if the device supports native 8-bit integer operations.
  xiiEnum<xiiGALDeviceFeatureState> m_ResourceBuffer8BitAccess;          ///< Indicates if the device supports reading and writing 8-bit types from buffers bound as shader resource or unordered access views.
  xiiEnum<xiiGALDeviceFeatureState> m_UniformBuffer8BitAccess;           ///< Indicates if the device supports reading 8-bit types from uniform buffers.
  xiiEnum<xiiGALDeviceFeatureState> m_ShaderResourceRuntimeArray;        ///< Indicates if the device supports runtime-sized shader arrays (eg. arrays without a specific size).This feature is always enabled in DirectX12 backend and can optionally be enabled in Vulkan backend. Run-time sized shader arrays are not available in other backends.
  xiiEnum<xiiGALDeviceFeatureState> m_WaveOp;                            ///< Indicates if the device supports wave ops or (DirectX 12) or subgroups (Vulkan).
  xiiEnum<xiiGALDeviceFeatureState> m_InstanceDataStepRate;              ///< Indicates if the device supports instance data step rates other than 1.
  xiiEnum<xiiGALDeviceFeatureState> m_NativeFence;                       ///< Indicates if the device supports fence with Uint64 counter. Native fence can wait on GPU for a signal from CPU, can be enqueued for wait operation for any value. If not natively supported by the device, the fence is emulated where possible.
  xiiEnum<xiiGALDeviceFeatureState> m_TileShaders;                       ///< Indicates if the device supports tile shaders.
  xiiEnum<xiiGALDeviceFeatureState> m_TransferQueueTimestampQueries;     ///< Indicates if the device supports timestamp and duration queries in transfer queues.
  xiiEnum<xiiGALDeviceFeatureState> m_VariableRateShading;               ///< Indicates if the device supports variable rate shading.
  xiiEnum<xiiGALDeviceFeatureState> m_SparseResources;                   ///< Indicates if the device supports sparse (aka. tiled or partially resident) resources.
  xiiEnum<xiiGALDeviceFeatureState> m_SubpassFramebufferFetch;           ///< Indicates if the device supports framebuffer fetch for input attachments.
  xiiEnum<xiiGALDeviceFeatureState> m_TextureComponentSwizzle;           ///< Indicates if the device supports texture component swizzle.
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
