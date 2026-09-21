/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Size.h>

#include <GraphicsFoundation/Declarations/Constants.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

class xiiWindowBase;

/// This describes the device features.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceFeatures : public xiiHashableStruct<xiiGALDeviceFeatures>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALDeviceFeatureState> m_WireframeFill                      = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports wireframe fill mode.
  xiiEnum<xiiGALDeviceFeatureState> m_MultithreadedResourceCreation      = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports multithreaded resource creation.
  xiiEnum<xiiGALDeviceFeatureState> m_ComputeShaders                     = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports compute shaders.
  xiiEnum<xiiGALDeviceFeatureState> m_GeometryShaders                    = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports geometry shaders.
  xiiEnum<xiiGALDeviceFeatureState> m_Tessellation                       = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports tessellation.
  xiiEnum<xiiGALDeviceFeatureState> m_MeshShaders                        = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports mesh and amplification shaders.
  xiiEnum<xiiGALDeviceFeatureState> m_RayTracing                         = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports ray tracing.
  xiiEnum<xiiGALDeviceFeatureState> m_BindlessResources                  = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports bindless resources.
  xiiEnum<xiiGALDeviceFeatureState> m_OcclusionQueries                   = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports occlusion queries.
  xiiEnum<xiiGALDeviceFeatureState> m_BinaryOcclusionQueries             = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports binary occlusion queries.
  xiiEnum<xiiGALDeviceFeatureState> m_TimestampQueries                   = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports timestamp queries.
  xiiEnum<xiiGALDeviceFeatureState> m_PipelineStatisticsQueries          = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports pipeline statistics queries.
  xiiEnum<xiiGALDeviceFeatureState> m_DurationQueries                    = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports duration queries.
  xiiEnum<xiiGALDeviceFeatureState> m_DepthBiasClamp                     = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports depth bias clamp.
  xiiEnum<xiiGALDeviceFeatureState> m_DepthClamp                         = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports depth clamp.
  xiiEnum<xiiGALDeviceFeatureState> m_IndependentBlend                   = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports independent blend.
  xiiEnum<xiiGALDeviceFeatureState> m_DualSourceBlend                    = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports dual-source blend.
  xiiEnum<xiiGALDeviceFeatureState> m_MultiViewport                      = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports multi-viewport.
  xiiEnum<xiiGALDeviceFeatureState> m_TextureCompressionBC               = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports all BC-compressed formats.
  xiiEnum<xiiGALDeviceFeatureState> m_VertexPipelineUAVWritesAndAtomics  = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports writes to UAVs as well as atomic operations in vertex, tessellation, and geometry shader stages.
  xiiEnum<xiiGALDeviceFeatureState> m_PixelUAVWritesAndAtomics           = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports writes to UAVs as well as atomic operations in pixel shader stage.
  xiiEnum<xiiGALDeviceFeatureState> m_TextureUAVExtendedFormats          = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports all the extended UAV texture formats available in the shader code.
  xiiEnum<xiiGALDeviceFeatureState> m_ShaderFloat16                      = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports native 16-bit float operations. Note that there are separate features that indicate if device supports loading 16-bit floats from buffers and passing them between shader stages. See https://therealmjp.github.io/posts/shader-fp16/ more info.
  xiiEnum<xiiGALDeviceFeatureState> m_ResourceBuffer16BitAccess          = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports reading and writing 16-bit floats and integers from buffers bound as shader resource or unordered access views.
  xiiEnum<xiiGALDeviceFeatureState> m_UniformBuffer16BitAccess           = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports reading 16-bit floats and integers from uniform buffers.
  xiiEnum<xiiGALDeviceFeatureState> m_ShaderInputOutput16                = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports using 16-bit floats and integers as input/output of a shader entry point.
  xiiEnum<xiiGALDeviceFeatureState> m_ShaderInt8                         = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports native 8-bit integer operations.
  xiiEnum<xiiGALDeviceFeatureState> m_ResourceBuffer8BitAccess           = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports reading and writing 8-bit types from buffers bound as shader resource or unordered access views.
  xiiEnum<xiiGALDeviceFeatureState> m_UniformBuffer8BitAccess            = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports reading 8-bit types from uniform buffers.
  xiiEnum<xiiGALDeviceFeatureState> m_ShaderResourceRuntimeArray         = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports runtime-sized shader arrays (eg. arrays without a specific size).This feature is always enabled in DirectX12 backend and can optionally be enabled in Vulkan backend. Run-time sized shader arrays are not available in other backends.
  xiiEnum<xiiGALDeviceFeatureState> m_WaveOperation                      = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports wave operations or (DirectX 12) or subgroups (Vulkan).
  xiiEnum<xiiGALDeviceFeatureState> m_InstanceDataStepRate               = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports instance data step rates other than 1.
  xiiEnum<xiiGALDeviceFeatureState> m_NativeFence                        = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports fence with Uint64 counter. Native fence can wait on GPU for a signal from CPU, can be enqueued for wait operation for any value. If not natively supported by the device, the fence is emulated where possible.
  xiiEnum<xiiGALDeviceFeatureState> m_TileShaders                        = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports tile shaders.
  xiiEnum<xiiGALDeviceFeatureState> m_TransferQueueTimestampQueries      = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports timestamp and duration queries in transfer queues.
  xiiEnum<xiiGALDeviceFeatureState> m_VariableRateShading                = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports variable rate shading.
  xiiEnum<xiiGALDeviceFeatureState> m_SparseResources                    = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports sparse (aka. tiled or partially resident) resources.
  xiiEnum<xiiGALDeviceFeatureState> m_SubpassFramebufferFetch            = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports framebuffer fetch for input attachments.
  xiiEnum<xiiGALDeviceFeatureState> m_TextureComponentSwizzle            = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports texture component swizzle.
  xiiEnum<xiiGALDeviceFeatureState> m_NativeMultiDraw                    = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports a dedicated command that can be used to issue multiple draw calls with a single command (e.g. vkCmdDrawMultiExt).
  xiiEnum<xiiGALDeviceFeatureState> m_AsynchronousShaderCompilation      = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports asynchronous shader compilation.
  xiiEnum<xiiGALDeviceFeatureState> m_VertexShaderRenderTargetArrayIndex = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports SV_RenderTargetArrayIndex semantic in the vertex shader.
  xiiEnum<xiiGALDeviceFeatureState> m_DepthStencilResolve                = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports depth/stencil resolve operations.
  xiiEnum<xiiGALDeviceFeatureState> m_ExternalMemory                     = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports shared memory across multiple devices or APIs.
  xiiEnum<xiiGALDeviceFeatureState> m_ExternalSemaphore                  = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports shared semaphores across multiple devices or APIs.
  xiiEnum<xiiGALDeviceFeatureState> m_ExternalFence                      = xiiGALDeviceFeatureState::Disabled; ///< Indicates if the device supports shared fences across multiple devices or APIs.
};

/// This describes the optimized depth-stencil clear value.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDepthStencilClearValue : public xiiHashableStruct<xiiGALDepthStencilClearValue>
{
  XII_DECLARE_POD_TYPE();

  float    m_fDepth    = 1.0f; ///< Depth clear value.
  xiiUInt8 m_uiStencil = 0U;   ///< Stencil clear value.
};

/// This describes the optimized color clear value.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALOptimizedClearValue : public xiiHashableStruct<xiiGALOptimizedClearValue>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALResourceFormat> m_ResourceFormat = xiiGALResourceFormat::Unknown; ///< Texture format.
  xiiColor                      m_ClearColour    = xiiColor::Black;               ///< Render target clear value.
  xiiGALDepthStencilClearValue  m_DepthStencil;                                   ///< Depth stencil clear value.
};

/// This describes the swap chain creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSwapChainCreationDescription : public xiiHashableStruct<xiiGALSwapChainCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiWindowBase*                         m_pWindow           = nullptr;                                    ///< Pointer to the window class.
  xiiEnum<xiiGALResourceFormat>          m_ColorBufferFormat = xiiGALResourceFormat::RGBA8UNormalizedSRGB; ///< Back buffer format.
  xiiBitflags<xiiGALSwapChainUsageFlags> m_UsageFlags        = xiiGALSwapChainUsageFlags::RenderTarget;    ///< Swap chain usage flags.
  xiiEnum<xiiGALSurfaceTransform>        m_PreTransform      = xiiGALSurfaceTransform::Optimal;            ///< The transform, relative to the presentation engine's natural orientation which is applied to the image prior to presentation.
                                                                                                           ///
                                                                                                           ///  \note When xiiGALSurfaceTransform::Optimal is used, the engine will select the most optimal surface transformation. An application may request a specific transform and the engine will try to use that. If the transform is not available, the engine will select the most optimal transform. After the swap chain has been created, this member will contain the actual transform selected by the engine.
  xiiUInt32 m_uiBufferCount         = 2U;                                                                  ///< The number of buffers in the swap chain.
  float     m_fDefaultDepthValue    = 1.0f;                                                                ///< Default depth value, which is used as the optimized depth clear value in D3D12.
  xiiUInt8  m_uiDefaultStencilValue = 0U;                                                                  ///< Default stencil value, which is used as the optimized clear value in D3D12.
};

/// This describes the texture properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTextureProperties : public xiiHashableStruct<xiiGALTextureProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiMaxTexture1DDimension     = 0U;    ///< Maximum dimension (width) of a 1D texture, or 0 if 1D textures are not supported.
  xiiUInt32 m_uiMaxTexture1DArraySlices   = 0U;    ///< Maximum number of slices in a 1D texture array, or 0 if 1D texture arrays are not supported.
  xiiUInt32 m_uiMaxTexture2DDimension     = 0U;    ///< Maximum dimension (width or height) of a 2D texture.
  xiiUInt32 m_uiMaxTexture2DArraySlices   = 0U;    ///< Maximum number of slices in a 2D texture array, or 0 if 2D texture arrays are not supported.
  xiiUInt32 m_uiMaxTexture3DDimension     = 0U;    ///< Maximum dimension (width, height, or depth) of a 3D texture, or 0 if 3D textures are not supported.
  xiiUInt32 m_uiMaxTextureCubeDimension   = 0U;    ///< Maximum dimension (width or height) of a cube map face, or 0 if cube map textures are not supported.
  bool      m_bTexture2DMSSupported       = false; ///< Indicates if device supports 2D multi-sampled textures.
  bool      m_bTexture2DMSArraySupported  = false; ///< Indicates if device supports 2D multi-sampled texture arrays.
  bool      m_bTextureViewSupported       = false; ///< Indicates if device supports texture views.
  bool      m_bCubeMapArraysSupported     = false; ///< Indicates if device supports cube map arrays.
  bool      m_bTextureView2DOn3DSupported = false; ///< Indicates if device supports 2D views from 3D texture.
};

/// This describes the texture sampler properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSamplerProperties : public xiiHashableStruct<xiiGALSamplerProperties>
{
  XII_DECLARE_POD_TYPE();

  bool      m_bBorderSamplingModeSupported = false; ///< Indicates if device supports border texture addressing mode.
  xiiUInt32 m_uiMaxAnisotropy              = 1U;    ///< Maximum anisotropy level supported by the device. If anisotropic filtering is not supported by the device, this value is 1.
  bool      m_bLODBiasSupported            = false; ///< Indicates if device supports MIP load bias.
};

/// This describes the sampler properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALWaveOperationProperties : public xiiHashableStruct<xiiGALWaveOperationProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32                      m_uiMinSize             = 0U;                         ///< Minimum supported size of the wave.
  xiiUInt32                      m_uiMaxSize             = 0U;                         ///< Maximum supported size of the wave. If variable wave size is not supported then this value is equal to MinSize. Direct3D12 backend: requires shader model 6.6. Vulkan backend: requires VK_EXT_subgroup_size_control.
  xiiBitflags<xiiGALShaderType>  m_SupportedShaderStages = xiiGALShaderType::Unknown;  ///< Shader stages in which wave operations can be used.
  xiiBitflags<xiiGALWaveFeature> m_WaveFeatures          = xiiGALWaveFeature::Unknown; ///< Indicates which groups of wave operations are supported by this device.
};

/// This describes the buffer properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBufferProperties : public xiiHashableStruct<xiiGALBufferProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiConstantBufferAlignment         = 0U; ///< The minimum required alignment, in bytes, for the constant buffer offsets.
  xiiUInt32 m_uiStructuredBufferOffsetAlignment = 0U; ///< The minimum required alignment, in bytes, for the structured buffer offsets.
};

/// This describes the ray tracing properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingProperties : public xiiHashableStruct<xiiGALRayTracingProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32                                    m_uiMaxRecursionDepth        = 0U;                                    ///< The maximum ray tracing recursion depth.
  xiiUInt32                                    m_uiMaxRayGenThreads         = 0U;                                    ///< The maximum total number of ray generation threads in one dispatch.
  xiiUInt32                                    m_uiMaxInstancesPerTLAS      = 0U;                                    ///< The maximum number of instances in a top-level AS.
  xiiUInt32                                    m_uiMaxPrimitivesPerBLAS     = 0U;                                    ///< The maximum number of primitives in a bottom-level AS.
  xiiUInt32                                    m_uiMaxGeometriesPerBLAS     = 0U;                                    ///< The maximum number of geometries in a bottom-level AS.
  xiiUInt32                                    m_uiVertexBufferAlignment    = 0U;                                    ///< The minimum alignment for the BLAS vertex buffer offset.
  xiiUInt32                                    m_uiIndexBufferAlignment     = 0U;                                    ///< The minimum alignment for the BLAS index buffer offset.
  xiiUInt32                                    m_uiTransformBufferAlignment = 0U;                                    ///< The minimum alignment for the BLAS transform buffer offset.
  xiiUInt32                                    m_uiBoxBufferAlignment       = 0U;                                    ///< The minimum alignment for the BLAS box buffer offset.
  xiiUInt32                                    m_uiScratchBufferAlignment   = 0U;                                    ///< The minimum alignment for the BLAS scratch buffer offset.
  xiiUInt32                                    m_uiInstanceBufferAlignment  = 0U;                                    ///< The minimum alignment for the BLAS instance buffer offset.
  xiiBitflags<xiiGALRayTracingCapabilityFlags> m_CapabilityFlags            = xiiGALRayTracingCapabilityFlags::None; ///< Ray tracing capability flags. See xiiGALRayTracingCapabilityFlags;

  xiiUInt32 m_uiShaderGroupHandleSize    = 0U; ///< Internal usage.
  xiiUInt32 m_uiMaxShaderRecordStride    = 0U; ///< Internal usage.
  xiiUInt32 m_uiShaderGroupBaseAlignment = 0U; ///< Internal usage.
};

/// This describes the mesh shader properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMeshShaderProperties : public xiiHashableStruct<xiiGALMeshShaderProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiMaxThreadGroupCountX     = 0U; ///< The maximum number of mesh shader thread groups in X direction.
  xiiUInt32 m_uiMaxThreadGroupCountY     = 0U; ///< The maximum number of mesh shader thread groups in Y direction.
  xiiUInt32 m_uiMaxThreadGroupCountZ     = 0U; ///< The maximum number of mesh shader thread groups in Z direction.
  xiiUInt32 m_uiMaxThreadGroupTotalCount = 0U; ///< The total maximum number of mesh shader groups per draw command.
};

/// This describes the compute shader properties.
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

/// This describes the graphics device creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceCreationDescription : public xiiHashableStruct<xiiGALDeviceCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALGraphicsDeviceType>    m_GraphicsDeviceType = xiiGALGraphicsDeviceType::Null;        ///< Graphics API type of the device to create.
  xiiEnum<xiiGALDeviceValidationLevel> m_ValidationLevel    = xiiGALDeviceValidationLevel::Standard; ///< Validation level for the device. Higher validation levels may enable additional GPU-based validation and debugging features, which can help catch more issues during development, but may also have a performance impact. The optimal validation level depends on the specific needs of the application and the stage of development. For example, during early development or when debugging complex issues, a higher validation level may be beneficial. However, for performance testing or in production builds, a lower validation level or no validation may be more appropriate.
  xiiUInt32                            m_uiAdapterID        = XII_GAL_DEFAULT_ADAPTER_ID;            ///< Adapter ID of the device to create. This is only used when there are multiple adapters available and the application wants to specify which one to use. The optimal adapter ID depends on the specific needs of the application and the hardware configuration of the system. In most cases, using the default adapter (XII_GAL_DEFAULT_ADAPTER_ID) is sufficient, as it typically corresponds to the primary GPU in the system. However, in systems with multiple GPUs, such as those with both integrated and discrete graphics, or in multi-GPU setups, specifying a particular adapter ID may be necessary to ensure that the application uses the desired GPU for rendering.
  xiiGALDeviceFeatures                 m_DeviceFeatures;                                             ///< Device features that the application requires. The optimal device features depend on the specific needs of the application and the capabilities of the target hardware. For example, if the application relies heavily on compute shaders, it would be important to require support for compute shaders in the device features. Similarly, if the application uses ray tracing, it would need to require support for ray tracing. It's important to carefully consider which features are necessary for the application's functionality and performance requirements, as requiring unsupported features may lead to device creation failure or suboptimal performance.
};

/// This describes the device memory properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceMemoryProperties : public xiiHashableStruct<xiiGALDeviceMemoryProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt64                        m_uiLocalMemory               = 0U;                        ///< The amount of local video memory that is inaccessible by CPU, in bytes.
  xiiUInt64                        m_uiHostVisibleMemory         = 0U;                        ///< The amount of host-visible memory that can be accessed by CPU and is visible by GPU, in bytes.
  xiiUInt64                        m_uiUnifiedMemory             = 0U;                        ///< The amount of unified memory that can be directly accessed by both CPU and GPU, in bytes.
  xiiUInt64                        m_uiMaxMemoryAllocation       = 0U;                        ///< Maximum size of a continuous memory block.
  xiiBitflags<xiiGALCPUAccessFlag> m_UnifiedMemoryCPUAccessFlags = xiiGALCPUAccessFlag::None; ///< Supported access types for the unified memory.
  xiiBitflags<xiiGALBindFlags>     m_MemorylessTextureBindFlags  = xiiGALBindFlags::None;     ///< Indicates if device supports color and depth attachments in on-chip memory. If supported, it will be combination of the following flags: RenderTarget, DepthStencil, InputAttachment.
};

/// This describes a combination of a shading rate and multi-sampling mode.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShadingRateMode : public xiiHashableStruct<xiiGALShadingRateMode>
{
  XII_DECLARE_POD_TYPE();

  xiiBitflags<xiiGALShadingRateFlags> m_ShadingRate = xiiGALShadingRateFlags::_1X1; ///< The supported shading rate.
  xiiBitflags<xiiGALSampleCount>      m_SampleBits  = xiiGALSampleCount::None;      ///< The combination of supported sample counts.
};

/// This describes the shading rate properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShadingRateProperties
{
  xiiHybridArray<xiiGALShadingRateMode, 2U>     m_Modes;                                                                    ///< Contains an array of supported combinations of shading rate and number of samples. The array is sorted in ascending order.
  xiiBitflags<xiiGALShadingRateCapabilityFlags> m_CapabilityFlags            = xiiGALShadingRateCapabilityFlags::None;      ///< Shading rate capability flags.
  xiiBitflags<xiiGALShadingRateCombinerFlags>   m_CombinerFlags              = xiiGALShadingRateCombinerFlags::PassThrough; ///< Combination of all supported shading rate combiners.
  xiiEnum<xiiGALShadingRateFormat>              m_Format                     = xiiGALShadingRateFormat::Unknown;            ///< Indicates which shading rate texture format is used by this device.
  xiiEnum<xiiGALShadingRateTextureAccess>       m_TextureAccess              = xiiGALShadingRateTextureAccess::Unknown;     ///< Shading rate texture access type.
  xiiBitflags<xiiGALBindFlags>                  m_BindFlags                  = xiiGALBindFlags::None;                       ///< Indicates which bind flags are allowed for shading rate texture.
  xiiSizeU32                                    m_MinTileSize                = xiiSizeU32(0U, 0U);                          ///< Minimum supported tile size. Shading rate texture size must be less than or equal to (Framebuffer Size / MaxTileSize).
  xiiSizeU32                                    m_MaxTileSize                = xiiSizeU32(0U, 0U);                          ///< Maximum supported tile size. Shading rate texture size must be greater than or equal to (Framebuffer Size / MaxTileSize).
  xiiUInt32                                     m_uiMaxSubSampledArraySlices = 0U;                                          ///< Maximum size of the texture array created with texture subsampled flag.
};

/// This describes the draw command properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDrawCommandProperties : public xiiHashableStruct<xiiGALDrawCommandProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiBitflags<xiiGALDrawCommandCapabilityFlags> m_CapabilityFlags        = xiiGALDrawCommandCapabilityFlags::None; ///< Draw command capability flags.
  xiiUInt32                                     m_uiMaxIndexValue        = 0U;                                     ///< Maximum supported index value for index buffer.
  xiiUInt32                                     m_uiMaxDrawIndirectCount = 0U;                                     ///< Maximum supported draw commands counter for indirect and indexed indirect draw commands.
};

/// This describes the sparse memory properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSparseResourceProperties : public xiiHashableStruct<xiiGALSparseResourceProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt64                                        m_uiAddressSpaceSize  = 0U;                                        ///< The total amount of address space, in bytes, available for sparse resources.
  xiiUInt64                                        m_uiResourceSpaceSize = 0U;                                        ///< The total amount of address space, in bytes, available for a single resource.
  xiiBitflags<xiiGALSparseResourceCapabilityFlags> m_CapabilityFlags     = xiiGALSparseResourceCapabilityFlags::None; ///< Sparse resource capability flags.
  xiiUInt32                                        m_uiStandardBlockSize = 0U;                                        ///< Size of the standard sparse memory block in bytes.
  xiiBitflags<xiiGALBindFlags>                     m_BindFlags           = xiiGALBindFlags::None;                     ///< Allowed bind flags for sparse buffer.
};

/// This describes the command queue properties.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandQueueProperties : public xiiHashableStruct<xiiGALCommandQueueProperties>
{
  XII_DECLARE_POD_TYPE();

  xiiBitflags<xiiGALCommandQueueFlags> m_Flags                      = xiiGALCommandQueueFlags::None; ///< Indicates which type of commands are supported by this queue.
  xiiUInt32                            m_uiMaxDeviceContexts        = 0U;                            ///< The maximum number of command queues that may be created for this queue.
  xiiUInt32                            m_TextureCopyGranularity[3U] = {};                            ///< Defines required texture offset and size alignment for copy operations in transfer queues.
};

/// Describes the resource and feature‐limits exposed the device.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceLimits : public xiiHashableStruct<xiiGALDeviceLimits>
{
  xiiUInt32 m_uiMaxConstantBuffers    = 0U; ///< Maximum number of constant (uniform) buffers that can be bound at once.
  xiiUInt32 m_uiMaxVertexBuffers      = 0U; ///< Maximum number of vertex buffers that can be bound at once.
  xiiUInt32 m_uiMaxSamplers           = 0U; ///< Maximum number of samplers that can be bound at once.
  xiiUInt32 m_uiMaxRenderTargets      = 0U; ///< Maximum number of color attachments (render targets) supported.
  xiiUInt32 m_uiMaxViewports          = 0U; ///< Maximum number of viewports that can be set in a single draw call.
  xiiUInt32 m_uiMaxShadingRateCombos  = 0U; ///< Number of variable‐rate shading combinations supported.
  xiiUInt32 m_uiShadingRateXShift     = 0U; ///< Bit shift for extracting the X‐axis rate from a combined shading‐rate enum.
  xiiUInt32 m_uiMaxResourceSignatures = 0U; ///< Maximum number of descriptor‐set layouts (or root‐signature slots) per pipeline.

  //-------------------------------------------------------------------------
  // Descriptor‐set / buffer size limits
  //-------------------------------------------------------------------------

  xiiUInt32 m_uiMaxPushConstantsSize     = 0U; ///< Maximum push‐constant size (in bytes).
  xiiUInt32 m_uiMaxUniformBufferRange    = 0U; ///< Maximum uniform buffer size (in bytes).
  xiiUInt32 m_uiMaxStorageBufferRange    = 0U; ///< Maximum storage buffer size (in bytes).
  xiiUInt32 m_uiMaxSampledImages         = 0U; ///< Maximum number of sampled images per descriptor set.
  xiiUInt32 m_uiMaxStorageImages         = 0U; ///< Maximum number of storage images per descriptor set.
  xiiUInt32 m_uiMaxStorageBuffers        = 0U; ///< Maximum number of storage buffers per descriptor set.
  xiiUInt32 m_uiMaxCombinedImageSamplers = 0U; ///< Maximum number of combined image+sampler descriptors.

  //-------------------------------------------------------------------------
  // Compute shader limits
  //-------------------------------------------------------------------------

  xiiUInt32 m_uiMaxComputeWorkGroupInvocations = 0U; ///< Maximum total invocations in a single compute workgroup.
  xiiUInt32 m_uiMaxComputeWorkGroupSizeX       = 0U; ///< Maximum size of a compute workgroup in the X dimension.
  xiiUInt32 m_uiMaxComputeWorkGroupSizeY       = 0U; ///< Maximum size of a compute workgroup in the Y dimension.
  xiiUInt32 m_uiMaxComputeWorkGroupSizeZ       = 0U; ///< Maximum size of a compute workgroup in the Z dimension.
  xiiUInt32 m_uiMaxComputeSharedMemorySize     = 0U; ///< Maximum shared memory size (in bytes) available to a compute workgroup.
};

/// Describes the properties and capabilities of a graphics device adapter.
///
/// This struct provides detailed information about a physical graphics adapter, including its vendor identity, hardware features, and supported limits.
/// Populated during adapter enumeration by the backend API (e.g., Vulkan or D3D12).
struct XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsDeviceAdapterDescription
{
  xiiString                                        m_sAdapterName;                                              ///< Human-readable name of the adapter (e.g., "NVIDIA GeForce RTX 5090").
  xiiEnum<xiiGALDeviceAdapterType>                 m_Type               = xiiGALDeviceAdapterType::Unknown;     ///< High-level type of the adapter (e.g., discrete, integrated, virtual).
  xiiEnum<xiiGALGraphicsAdapterVendor>             m_Vendor             = xiiGALGraphicsAdapterVendor::Unknown; ///< Hardware vendor classification (e.g., NVIDIA, AMD, Intel).
  xiiUInt32                                        m_uiVendorID         = 0U;                                   ///< PCI vendor identifier (e.g., 0x10DE for NVIDIA).
  xiiUInt32                                        m_uiDeviceID         = 0U;                                   ///< PCI device identifier (specific to the GPU model).
  xiiUInt32                                        m_uiVideoOutputCount = 0U;                                   ///< Number of available video outputs connected to the adapter.
  xiiGALDeviceMemoryProperties                     m_MemoryProperties;                                          ///< Device-local memory capacity and heap organization.
  xiiGALRayTracingProperties                       m_RayTracingProperties;                                      ///< Ray tracing hardware capabilities (acceleration structures, shader support).
  xiiGALWaveOperationProperties                    m_WaveOperationProperties;                                   ///< Wave-level execution properties (e.g., wave size, lane counts).
  xiiGALBufferProperties                           m_BufferProperties;                                          ///< Buffer-related capabilities (alignment, max sizes, usage flags).
  xiiGALTextureProperties                          m_TextureProperties;                                         ///< Texture support details (dimensionality, sample counts, formats).
  xiiGALSamplerProperties                          m_SamplerProperties;                                         ///< Sampler configuration limits (filtering modes, address modes).
  xiiGALMeshShaderProperties                       m_MeshShaderProperties;                                      ///< Mesh shader capabilities and pipeline limits.
  xiiGALShadingRateProperties                      m_ShadingRateProperties;                                     ///< Variable-rate shading granularity and compatibility info.
  xiiGALComputeShaderProperties                    m_ComputeShaderProperties;                                   ///< Compute shader configuration limits (workgroup size, shared memory).
  xiiGALDrawCommandProperties                      m_DrawCommandProperties;                                     ///< Draw command constraints (indirect count, multi-viewport support).
  xiiGALSparseResourceProperties                   m_SparseResourceProperties;                                  ///< Sparse resource support flags (residency, binding granularity).
  xiiGALDeviceFeatures                             m_Features;                                                  ///< Indicates which optional GPU features are supported (e.g., dynamic rendering, descriptor indexing).
  xiiGALDeviceLimits                               m_DeviceLimits;                                              ///< Resource binding and execution limits shared across pipeline stages. This includes descriptor count limits, push constant ranges, and compute workgroup parameters.
  xiiHybridArray<xiiGALCommandQueueProperties, 3U> m_CommandQueueProperties;                                    ///< List of command queue families and their properties (e.g., graphics, compute, transfer). Each entry describes the capabilities and priorities of a queue family available on this adapter.
};

/// This describes the graphics abstraction layer device events.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceEvent : public xiiHashableStruct<xiiGALDeviceEvent>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALDeviceEventType> m_Type    = xiiGALDeviceEventType::Unknown;
  xiiGALDevice*                  m_pDevice = nullptr;
};

/// This describes the invariant texture format attributes. These attributes are intrinsic to the texture format itself and do not depend on the format support.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALResourceFormatDescription : public xiiHashableStruct<xiiGALResourceFormatDescription>
{
  XII_DECLARE_POD_TYPE();

  /// Returns true if the format is block-compressed (BC, ETC, ASTC, etc.).
  XII_ALWAYS_INLINE bool IsCompressed() const { return m_ComponentType == xiiGALResourceFormatComponentType::Compressed; }

  /// Returns the number of bytes per texel (non-compressed) or per block (compressed).
  XII_ALWAYS_INLINE xiiUInt32 GetElementSize() const { return m_uiComponentSize * (IsCompressed() ? 1 : m_uiComponentCount); }

  /// Returns the number of bits per texel (non-compressed) or per block (compressed).
  XII_ALWAYS_INLINE xiiUInt32 GetBitsPerPixel() const { return GetElementSize() * 8U; }

  /// Returns the width of a compression block (4 for BC formats).
  XII_ALWAYS_INLINE xiiUInt32 GetBlockWidth() const { return IsCompressed() ? m_uiBlockWidth : 1; }

  /// Returns the height of a compression block (4 for BC formats).
  XII_ALWAYS_INLINE xiiUInt32 GetBlockHeight() const { return IsCompressed() ? m_uiBlockHeight : 1; }

  /// Returns how many blocks are needed horizontally for a given width.
  XII_ALWAYS_INLINE xiiUInt32 GetBlockCountX(xiiUInt32 uiWidth) const { return (uiWidth + GetBlockWidth() - 1) / GetBlockWidth(); }

  /// Returns how many blocks are needed vertically for a given height.
  XII_ALWAYS_INLINE xiiUInt32 GetBlockCountY(xiiUInt32 uiHeight) const { return (uiHeight + GetBlockHeight() - 1) / GetBlockHeight(); }

  /// Returns the number of bytes in one row of texels or blocks.
  XII_ALWAYS_INLINE xiiUInt32 GetRowPitch(xiiUInt32 uiWidth) const
  {
    if (!IsCompressed())
    {
      return uiWidth * GetElementSize();
    }
    return GetBlockCountX(uiWidth) * GetElementSize();
  }

  /// Returns the number of bytes in one 2D slice (height * rowPitch).
  XII_ALWAYS_INLINE xiiUInt64 GetSlicePitch(xiiUInt32 uiWidth, xiiUInt32 uiHeight) const { return GetRowPitch(uiWidth) * (IsCompressed() ? GetBlockCountY(uiHeight) : uiHeight); }

  /// Returns the number of texels per block (16 for BC1–BC5).
  XII_ALWAYS_INLINE xiiUInt32 GetTexelsPerBlock() const { return GetBlockWidth() * GetBlockHeight(); }

  /// Returns true if the format is typeless.
  XII_ALWAYS_INLINE bool IsTypeless() const { return m_bIsTypeless; }

  /// Returns true if the format is UNorm or SNorm.
  XII_ALWAYS_INLINE bool IsNormalized() const { return m_ComponentType == xiiGALResourceFormatComponentType::UnsignedNormalized || m_ComponentType == xiiGALResourceFormatComponentType::SignedNormalized; }

  xiiEnum<xiiGALResourceFormat>              m_Format           = xiiGALResourceFormat::Unknown;                ///< Texture format.
  xiiUInt8                                   m_uiComponentSize  = 0U;                                           ///< The size of one component in bytes.
  xiiUInt8                                   m_uiComponentCount = 0U;                                           ///< The number of components.
  xiiEnum<xiiGALResourceFormatComponentType> m_ComponentType    = xiiGALResourceFormatComponentType::Undefined; ///< The component type.
  bool                                       m_bIsTypeless      = false;                                        ///< Indicates whether the format is a typeless format.
  xiiUInt8                                   m_uiBlockWidth     = 0U;                                           ///< For block-compressed formats, the compression block width.
  xiiUInt8                                   m_uiBlockHeight    = 0U;                                           ///< For block-compressed formats, the compression block height.
};

/// This describes the multi-planar format attributes. These attributes are intrinsic to the multi-planar format itself and do not depend on the format support.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMultiPlanarFormatDescription : public xiiHashableStruct<xiiGALMultiPlanarFormatDescription>
{
  XII_DECLARE_POD_TYPE();

  struct Plane
  {
    xiiEnum<xiiGALResourceFormat> m_SubFormat         = xiiGALResourceFormat::Unknown; ///< Format of this plane (e.g., R8_UNORM, R8G8_UNORM, R16_UNORM).
    xiiUInt8                      m_uiBytesPerElement = 0U;                            ///< Bytes per element in this plane.
    float                         m_fWidthFactor      = 1.0f;                          ///< Width scaling relative to full resolution (0.5 for chroma).
    float                         m_fHeightFactor     = 1.0f;                          ///< Height scaling relative to full resolution (0.5 for chroma).
  };

  /// Returns the number of planes in this multi-planar format.
  XII_ALWAYS_INLINE xiiUInt32 GetPlaneCount() const { return m_Planes.GetCount(); }

  /// Returns true if the specified plane index is valid.
  XII_ALWAYS_INLINE bool HasPlane(xiiUInt32 uiPlane) const { return uiPlane < m_Planes.GetCount(); }

  /// Returns the description of the specified plane.
  XII_ALWAYS_INLINE const Plane& GetPlane(xiiUInt32 uiPlane) const
  {
    XII_ASSERT_DEV(HasPlane(uiPlane), "Plane index ({}) out of range [0, {}).", uiPlane, m_Planes.GetCount());

    return m_Planes[uiPlane];
  }

  /// Returns the width of the specified plane, given the full resolution width.
  XII_ALWAYS_INLINE xiiUInt32 GetPlaneWidth(xiiUInt32 uiFullWidth, xiiUInt32 uiPlane) const
  {
    XII_ASSERT_DEV(HasPlane(uiPlane), "Plane index ({}) out of range [0, {}).", uiPlane, m_Planes.GetCount());

    return static_cast<xiiUInt32>(uiFullWidth * m_Planes[uiPlane].m_fWidthFactor);
  }

  /// Returns the height of the specified plane, given the full resolution height.
  XII_ALWAYS_INLINE xiiUInt32 GetPlaneHeight(xiiUInt32 uiFullHeight, xiiUInt32 uiPlane) const
  {
    XII_ASSERT_DEV(HasPlane(uiPlane), "Plane index ({}) out of range [0, {}).", uiPlane, m_Planes.GetCount());

    return static_cast<xiiUInt32>(uiFullHeight * m_Planes[uiPlane].m_fHeightFactor);
  }

  /// Returns the row pitch (in bytes) of the specified plane, given the full resolution width.
  XII_ALWAYS_INLINE xiiUInt32 GetPlaneRowPitch(xiiUInt32 uiFullWidth, xiiUInt32 uiPlane) const
  {
    XII_ASSERT_DEV(HasPlane(uiPlane), "Plane index ({}) out of range [0, {}).", uiPlane, m_Planes.GetCount());

    const Plane&    p       = m_Planes[uiPlane];
    const xiiUInt32 uiWidth = GetPlaneWidth(uiFullWidth, uiPlane);

    return uiWidth * p.m_uiBytesPerElement;
  }

  /// Returns the slice pitch (in bytes) of the specified plane, given the full resolution width and height.
  XII_ALWAYS_INLINE xiiUInt64 GetPlaneSlicePitch(xiiUInt32 uiFullWidth, xiiUInt32 uiFullHeight, xiiUInt32 uiPlane) const
  {
    XII_ASSERT_DEV(HasPlane(uiPlane), "Plane index ({}) out of range [0, {}).", uiPlane, m_Planes.GetCount());

    const xiiUInt32 uiHeight = GetPlaneHeight(uiFullHeight, uiPlane);

    return static_cast<xiiUInt64>(GetPlaneRowPitch(uiFullWidth, uiPlane)) * uiHeight;
  }

  /// Returns true if the multi-planar format description is valid.
  XII_ALWAYS_INLINE bool IsValid() const
  {
    if (!xiiGALResourceFormat::IsMultiplanar(m_Format))
      return false;

    if (m_Planes.GetCount() == 0)
      return false;

    for (xiiUInt32 i = 0; i < m_Planes.GetCount(); ++i)
    {
      const Plane& p = m_Planes[i];
      if (p.m_SubFormat == xiiGALResourceFormat::Unknown || p.m_uiBytesPerElement == 0 || p.m_fWidthFactor <= 0.0f || p.m_fHeightFactor <= 0.0f)
      {
        return false;
      }
    }

    return true;
  }

  /// Returns the total size (in bytes) of all planes, given the full resolution width and height.
  XII_ALWAYS_INLINE xiiUInt64 GetTotalSize(xiiUInt32 uiFullWidth, xiiUInt32 uiFullHeight) const
  {
    xiiUInt64 uiTotalSize = 0ULL;

    for (xiiUInt32 i = 0; i < m_Planes.GetCount(); ++i)
    {
      uiTotalSize += GetPlaneSlicePitch(uiFullWidth, uiFullHeight, i);
    }
    return uiTotalSize;
  }

  xiiEnum<xiiGALResourceFormat> m_Format = xiiGALResourceFormat::Unknown; ///< Multi-planar texture format.
  xiiStaticArray<Plane, 4>      m_Planes;                                 ///< Array of formats for each plane.
};

/// This describes the external memory description.
///
/// Used to import external memory handles into the graphics device. This is useful for interop scenarios where memory is shared between different APIs or processes.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALExternalMemoryDescription : public xiiHashableStruct<xiiGALExternalMemoryDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiBitflags<xiiGALExternalMemoryKind>  m_Type                    = xiiGALExternalMemoryKind::None;  ///< The type of external memory handle.
  xiiBitflags<xiiGALExternalMemoryFlags> m_Flags                   = xiiGALExternalMemoryFlags::None; ///< The usage flags for the external memory.
  xiiUInt64                              m_uiNativeHandle          = 0U;                              ///< Native external memory handle (e.g., HANDLE on Windows, file descriptor on Linux).
  xiiUInt64                              m_uiSize                  = 0U;                              ///< Size of the external memory in bytes.
  xiiUInt64                              m_uiProcessId             = 0U;                              ///< Process ID of the process that created the external memory handle. This is used for cross-process memory sharing.
  xiiUInt32                              m_uiMemoryTypeIndex       = 0U;                              ///< Memory type index that is compatible with the external memory handle. This is used to ensure that the imported memory can be used with the graphics device.
  xiiUInt64                              m_uiNativeSemaphoreHandle = 0U;                              ///< Native external semaphore handle (e.g., HANDLE on Windows, file descriptor on Linux). Used when importing semaphores for synchronization.
};
