#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Size.h>

#include <GraphicsFoundation/Declarations/Constants.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief This describes the device features.
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
  xiiUInt8 m_uiStencil = 0U;   ///< Stencil clear value.
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
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDisplayModeDescription : public xiiHashableStruct<xiiGALDisplayModeDescription>
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

  xiiSizeU32                             m_Resolution        = xiiSizeU32(0U, 0U);                        ///< Swap chain resolution.
  xiiEnum<xiiGALTextureFormat>           m_ColorBufferFormat = xiiGALTextureFormat::RGBA8UNormalizedSRGB; ///< Back buffer format.
  xiiEnum<xiiGALTextureFormat>           m_DepthBufferFormat = xiiGALTextureFormat::D32Float;             ///< Depth buffer format. Use Unknown format to create the swapchain without a depth buffer.
  xiiBitflags<xiiGALSwapChainUsageFlags> m_Usage             = xiiGALSwapChainUsageFlags::RenderTarget;   ///< Swap chain usage flags.
  xiiEnum<xiiGALSurfaceTransform>        m_PreTransform      = xiiGALSurfaceTransform::Optimal;           ///< The transform, relative to the presentation engine's natural orientation which is applied to the image prior to presentation.
                                                                                                          ///<
                                                                                                          ///< \note When xiiGALSurfaceTransform::Optimal is used, the engine will select the most optimal surface transformation.
                                                                                                          ///< An application may request a specific transform and the engine will try to use that. If the transform is not available, the engine will
                                                                                                          ///< select the most optimal transform. After the swapchain has been created, this member will contain the actual transform selected by the engine.
  xiiUInt32 m_uiBufferCount         = 2U;                                                                 ///< The number of buffers in the swap chain.
  float     m_fDepthStencilValue    = 1.0f;                                                               ///< Default depth value, which is used as the optimized depth clear value in D3D12.
  xiiUInt8  m_uiDefaultStencilValue = 0U;                                                                 ///< Default stencil value, which is used as the optimized clear value in D3D12.
  bool      m_bIsPrimary            = true;                                                               ///< This indicates if this swap chain is a primary swap chain.
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

/// \brief This describes the texture properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTextureProperties : public xiiHashableStruct<xiiGALTextureProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiMaxTexture1DDimension     = 0U;    ///< Maximum dimension (width) of a 1D texture, or 0 if 1D textures are not supported.
  xiiUInt32 m_uiMaxTexture1DArraySlices   = 0U;    ///< Maximum number of slices in a 1D texture array, or 0 if 1D texture arrays are not supported.
  xiiUInt32 m_uiMaxTexture2DDimension     = 0U;    ///< Maximum dimension (width or height) of a 2D texture.
  xiiUInt32 m_uiMaxTexture2DArraySlices   = 0U;    ///< Maximum number of slices in a 2D texture array, or 0 if 2D texture arrays are not supported.
  xiiUInt32 m_uiMaxTexture3DDimension     = 0U;    ///< Maximum dimension (width, height, or depth) of a 3D texture, or 0 if 3D textures are not supported.
  xiiUInt32 m_uiMaxTextureCubeDimension   = 0U;    ///< Maximum dimension (width or height) of a cubemap face, or 0 if cubemap textures are not supported.
  bool      m_bTexture2DMSSupported       = false; ///< Indicates if device supports 2D multisampled textures.
  bool      m_bTexture2DMSArraySupported  = false; ///< Indicates if device supports 2D multisampled texture arrays.
  bool      m_bTextureViewSupported       = false; ///< Indicates if device supports texture views.
  bool      m_bCubemapArraysSupported     = false; ///< Indicates if device supports cubemap arrays.
  bool      m_bTextureView2DOn3DSupported = false; ///< Indicates if device supports 2D views from 3D texture.
};

/// \brief This describes the texture sampler properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSamplerProperties : public xiiHashableStruct<xiiGALSamplerProperties>
{
  XII_DECLARE_POD_TYPE();

  bool m_bBorderSamplingModeSupported   = false; ///< Indicates if device supports border texture addressing mode.
  bool m_bAnisotropicFilteringSupported = false; ///< Indicates if device supports anisotropic filtering.
  bool m_bLODBiasSupported              = false; ///< Indicates if device supports MIP load bias.
};

/// \brief This describes the sampler properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALWaveOperationProperties : public xiiHashableStruct<xiiGALWaveOperationProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32                      m_uiMinSize = 0U;        ///< Minimum supported size of the wave.
  xiiUInt32                      m_uiMaxSize = 0U;        ///< Maximum supported size of the wave. If variable wave size is not supported then this value is equal to MinSize. Direct3D12 backend: requires shader model 6.6. Vulkan backend: requires VK_EXT_subgroup_size_control.
  xiiBitflags<xiiGALShaderStage> m_SupportedShaderStages; ///< Shader stages in which wave operations can be used.
  xiiBitflags<xiiGALWaveFeature> m_WaveFeatures;          ///< Indicates which groups of wave operations are supported by this device.
};

/// \brief This describes the buffer properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBufferProperties : public xiiHashableStruct<xiiGALBufferProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiConstantBufferAlignment         = 0U; ///< The minimum required alignment, in bytes, for the constant buffer offsets.
  xiiUInt32 m_uiStructuredBufferOffsetAlignment = 0U; ///< The minimum required alignment, in bytes, for the structured buffer offsets.
};

/// \brief This describes the ray tracing properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingProperties : public xiiHashableStruct<xiiGALRayTracingProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32                                    m_uiMaxRecursionDepth        = 0U; ///< The maximum ray tracing recursion depth.
  xiiUInt32                                    m_uiMaxRayGenThreads         = 0U; ///< The maximum total number of ray generation threads in one dispatch.
  xiiUInt32                                    m_uiMaxInstancesPerTLAS      = 0U; ///< The maximum number of instances in a top-level AS.
  xiiUInt32                                    m_uiMaxPrimitivesPerBLAS     = 0U; ///< The maximum number of primitives in a bottom-level AS.
  xiiUInt32                                    m_uiMaxGeometriesPerBLAS     = 0U; ///< The maximum number of geometries in a bottom-level AS.
  xiiUInt32                                    m_uiVertexBufferAlignment    = 0U; ///< The minimum alignment for the BLAS vertex buffer offset.
  xiiUInt32                                    m_uiIndexBufferAlignment     = 0U; ///< The minimum alignment for the BLAS index buffer offset.
  xiiUInt32                                    m_uiTransformBufferAlignment = 0U; ///< The minimum alignment for the BLAS transform buffer offset.
  xiiUInt32                                    m_uiBoxBufferAlignment       = 0U; ///< The minimum alignment for the BLAS box buffer offset.
  xiiUInt32                                    m_uiScratchBufferAlignment   = 0U; ///< The minimum alignment for the BLAS scratch buffer offset.
  xiiUInt32                                    m_uiInstanceBufferAlignment  = 0U; ///< The minimum alignment for the BLAS instance buffer offset.
  xiiBitflags<xiiGALRayTracingCapabilityFlags> m_CapabilityFlags;                 ///< Ray tracing capability flags. See xiiGALRayTracingCapabilityFlags;

  // Internal usage.
  xiiUInt32 m_uiShaderGroupHandleSize    = 0u;
  xiiUInt32 m_uiMaxShaderRecordStride    = 0u;
  xiiUInt32 m_uiShaderGroupBaseAlignment = 0u;
};

/// \brief This describes the mesh shader properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMeshShaderProperties : public xiiHashableStruct<xiiGALMeshShaderProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiMaxTaskCount = 0U; ///< The maximum number of mesh shader tasks per draw command.
};

/// \brief This describes the compute shader properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALComputeShaderProperties : public xiiHashableStruct<xiiGALComputeShaderProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiSharedMemorySize          = 0U; ///< Amount of shared memory available to threads in one group.
  xiiUInt32 m_uiMaxThreadGroupInvocations = 0U; ///< The total maximum number of threads in one group.

  xiiUInt32 m_uiMaxThreadGroupSizeX = 0U; ///< The maximum number of threads in group X dimension.
  xiiUInt32 m_uiMaxThreadGroupSizeY = 0U; ///< The maximum number of threads in group Y dimension.
  xiiUInt32 m_uiMaxThreadGroupSizeZ = 0U; ///< The maximum number of threads in group Z dimension.

  xiiUInt32 m_uiMaxThreadGroupCountX = 0U; ///< The maximum number of thread groups that can be dispatched in X dimension.
  xiiUInt32 m_uiMaxThreadGroupCountY = 0U; ///< The maximum number of thread groups that can be dispatched in Y dimension.
  xiiUInt32 m_uiMaxThreadGroupCountZ = 0U; ///< The maximum number of thread groups that can be dispatched in Z dimension.
};

/// \brief This describes the normalized device coordinates attribute.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALNormalizedDeviceCoordinates : public xiiHashableStruct<xiiGALNormalizedDeviceCoordinates>
{
  XII_DECLARE_POD_TYPE();

  float m_fMinZ          = 0.0f;
  float m_fZToDepthScale = 0.0f;
  float m_fYToVScale     = 0.0f;

  /// \brief Returns ZtoDepthBias such that given NDC z coordinate, depth value can be computed as d = z * ZtoDepthScale + ZtoDepthBias.
  float GetZtoDepthBias() const;
};

/// \brief This describes the graphics device creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsDeviceCreationDescription : public xiiHashableStruct<xiiGALGraphicsDeviceCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALGraphicsDeviceType> m_GraphicsDeviceType;
  xiiGALDeviceFeatures              m_DeviceFeatures;
  xiiGALNormalizedDeviceCoordinates m_DeviceNormalizedCoordinates;
};

/// \brief This describes the device memory properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceMemoryProperties : public xiiHashableStruct<xiiGALDeviceMemoryProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt64                        m_uiLocalMemory         = 0U;  ///< The amount of local video memory that is inaccessible by CPU, in bytes.
  xiiUInt64                        m_uiHostVisibleMemory   = 0U;  ///< The amount of host-visible memory that can be accessed by CPU and is visible by GPU, in bytes.
  xiiUInt64                        m_uiUnifiedMemory       = 0U;  ///< The amount of unified memory that can be directly accessed by both CPU and GPU, in bytes.
  xiiUInt64                        m_uiMaxMemoryAllocation = 0U;  ///< Maximum size of a continuous memory block.
  xiiBitflags<xiiGALCPUAccessFlag> m_UnifiedMemoryCPUAccessFlags; ///< Supported access types for the unified memory.
  xiiBitflags<xiiGALBindFlags>     m_MemoryleessTextureBindFlags; ///< Indicates if device supports color and depth attachments in on-chip memory.
                                                                  ///< If supported, it will be combination of the following flags: RenderTarget, DepthStencil, InputAttachment.
};

/// \brief This describes a combination of a shading rate and multi-sampling mode.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShadingRateMode : public xiiHashableStruct<xiiGALShadingRateMode>
{
  XII_DECLARE_POD_TYPE();

  xiiBitflags<xiiGALShadingRate> m_ShadingRate; ///< The supported shading rate.
  xiiBitflags<xiiGALSampleCount> m_SampleBits;  ///< The combination of supported sample counts.
};

/// \brief This describes the shading rate properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShadingRateProperties : public xiiHashableStruct<xiiGALShadingRateProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiGALShadingRateMode                         m_Mode[XII_GAL_MAX_SHADING_RATE];                  ///< Contains an array of supported combinations of shading rate and number of samples. The array is sorted in ascending order.
  xiiUInt8                                      m_uiCount = 0U;                                    ///< The number of valid elements in ShadingRates array.
  xiiBitflags<xiiGALShadingRateCapabilityFlags> m_CapabilityFlags;                                 ///< Shading rate capability flags.
  xiiBitflags<xiiGALShadingRateCombiner>        m_CombinerFlags;                                   ///< Combination of all supported shading rate combiners.
  xiiEnum<xiiGALShadingRateFormat>              m_Format;                                          ///< Indicates which shading rate texture format is used by this device.
  xiiEnum<xiiGALShadingRateTextureAccess>       m_TextureAccess;                                   ///< Shading rate texture access type.
  xiiBitflags<xiiGALBindFlags>                  m_BindFlags;                                       ///< Indicates which bind flags are allowed for shading rate texture.
  xiiSizeU32                                    m_MinTileSize                = xiiSizeU32(0U, 0U); ///< Minimum supported tile size. Shading rate texture size must be less than or equal to (Framebuffer Size / MaxTileSize).
  xiiSizeU32                                    m_MaxTileSize                = xiiSizeU32(0U, 0U); ///< Maximum supported tile size. Shading rate texture size must be greater than or equal to (Framebuffer Size / MaxTileSize).
  xiiUInt32                                     m_uiMaxSubSampledArraySlices = 0U;                 ///< Maximum size of the texture array created with texture subsampled flag.
};

/// \brief This describes the draw command properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDrawCommandProperties : public xiiHashableStruct<xiiGALDrawCommandProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiBitflags<xiiGALDrawCommandCapabilityFlags> m_CapabilityFlags;             ///< Draw command capability flags.
  xiiUInt32                                     m_uiMaxIndexValue        = 0U; ///< Maximum supported index value for index buffer.
  xiiUInt32                                     m_uiMaxDrawIndirectCount = 0U; ///< Maximum supported draw commands counter for indirect and indexed indirect draw commands.
};

/// \brief This describes the sparse memory properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSparseResourceProperties : public xiiHashableStruct<xiiGALSparseResourceProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt64                                        m_uiAddressSpaceSize  = 0U; ///< The total amount of address space, in bytes, available for sparse resources.
  xiiUInt64                                        m_uiResourceSpaceSize = 0U; ///< The total amount of address space, in bytes, available for a single resource.
  xiiBitflags<xiiGALSparseResourceCapabilityFlags> m_CapabilityFlags;          ///< Sparse resource capability flags.
  xiiUInt32                                        m_uiStandardBlockSize;      ///< Size of the standard sparse memory block in bytes.
  xiiBitflags<xiiGALBindFlags>                     m_BindFlags;                ///< Allowed bind flags for sparse buffer.
};

/// \brief This describes the command queue properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandQueueProperties : public xiiHashableStruct<xiiGALCommandQueueProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiBitflags<xiiGALCommandQueueType> m_Type;                           ///< Indicates which type of commands are supported by this queue.
  xiiUInt32                           m_MaxDeviceContexts;              ///< The maximum number of immediate contexts that may be created for this queue.
  xiiUInt32                           m_TextureCopyGranularity[3] = {}; ///< Defines required texture offset and size alignment for copy operations in transfer queues.
};

/// \brief This describes the graphics adapter properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGraphicsDeviceAdapterDescription : public xiiHashableStruct<xiiGraphicsDeviceAdapterDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiString                            m_sAdapterName;                                            ///< A string that contains the adapter description.
  xiiEnum<xiiGALDeviceAdapterType>     m_Type;                                                    ///< Adapter type.
  xiiEnum<xiiGALGraphicsAdapterVendor> m_Vendor;                                                  ///< Adapter vendor.
  xiiUInt32                            m_uiVendorID         = 0U;                                 ///< The PCI ID of the hardware vendor (if available).
  xiiUInt32                            m_uiDeviceID         = 0U;                                 ///< The PCI ID of the hardware device (if available).
  xiiUInt32                            m_uiVideoOutputCount = 0U;                                 ///< Number of video outputs this adapter has (if available).
  xiiGALDeviceMemoryProperties         m_MemoryProperties;                                        ///< Device memory information.
  xiiGALRayTracingProperties           m_RayTracingProperties;                                    ///< Ray tracing properties.
  xiiGALWaveOperationProperties        m_WaveOperationProperties;                                 ///< Wave operation properties.
  xiiGALBufferProperties               m_BufferProperties;                                        ///< Buffer properties.
  xiiGALTextureProperties              m_TextureProperties;                                       ///< Texture properties.
  xiiGALSamplerProperties              m_SamplerProperties;                                       ///< Sampler properties.
  xiiGALMeshShaderProperties           m_MeshShaderProperties;                                    ///< Mesh shader properties.
  xiiGALShadingRateProperties          m_ShadingRateProperties;                                   ///< Shading rate properties.
  xiiGALComputeShaderProperties        m_ComputeShaderProperties;                                 ///< Compute shader properties.
  xiiGALDrawCommandProperties          m_DrawCommandProperties;                                   ///< Draw command properties.
  xiiGALSparseResourceProperties       m_SparseResourceProperties;                                ///< Sparse resource properties.
  xiiGALDeviceFeatures                 m_Features;                                                ///< Supported device features.
  xiiGALCommandQueueProperties         m_CommandQueueProperties[XII_GAL_MAX_ADAPTER_QUEUES] = {}; ///< An array of NumQueues command queues supported by this device.
  xiiUInt32                            m_AdapterQueuesCount                                 = 0U; ///< The number of queues in the command queue properties array.
};

/// \brief This describes the immediate context device creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALImmediateContextCreationDescription : public xiiHashableStruct<xiiGALImmediateContextCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiStringView                       m_sName;          ///< Context name.
  xiiUInt8                            m_uiQueueID = 0U; ///< Queue index.
  xiiEnum<xiiGALCommandQueuePriority> m_QueuePriority;  ///< Priority of the software queue created by the context.
};

/// \brief This describes the graphics abstraction layer device events.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceEvent : public xiiHashableStruct<xiiGALDeviceEvent>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALDeviceEventType> m_Type;

  class xiiGALDevice* m_pDevice = nullptr;
};

/// \brief This describes the invariant texture format attributes. These attributes are intrinsic to the texture format itself and do not depend on the format support.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTextureFormatDescription : public xiiHashableStruct<xiiGALTextureFormatDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALTextureFormat>              m_Format;                ///< Texture format.
  xiiUInt8                                  m_uiComponentSize  = 0U; ///< The size of one component in bytes.
  xiiUInt8                                  m_uiComponentCount = 0U; ///< The number of components.
  xiiEnum<xiiGALTextureFormatComponentType> m_ComponentType;         ///< The component type.
  bool                                      m_bIsTypeless   = false; ///< Indicates whether the format is a typeless format.
  xiiUInt8                                  m_uiBlockWidth  = 0U;    ///< For block-compressed formats, the compression block width.
  xiiUInt8                                  m_uiBlockHeight = 0U;    ///< For block-compressed formats, the compression block height.
};

#include <GraphicsFoundation/Declarations/Implementation/Descriptors_inl.h>
