/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Shader/Shader.h>
#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/States/DepthStencilState.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>
#include <GraphicsFoundation/States/RasterizerState.h>

/// This describes the shader variable property flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderVariableFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None             = 0U,         ///< Shader variable has no special properties.
    NoDynamicBuffers = XII_BIT(0), ///< This indicates that dynamic buffers will never be bound to the resource variable. This applies to uniform (constant) buffers, unordered access views, and shader resource views.
                                   ///<
                                   ///< \remarks This flag directly translates to the xiiGALPipelineResourceFlag::NoDynamicBuffers in the internal pipeline resource signature.
    InputAttachment = XII_BIT(1),  ///<          This indicates that the resource is an input attachment in general layout, which allows simultaneously reading from the resource through the input attachment and writing to it via color or depth-stencil attachment.
                                   ///<
                                   ///< \note This flag is only valid in Vulkan.

    Default = None
  };

  struct Bits
  {
    StorageType NoDynamicBuffers : 1;
    StorageType InputAttachment : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALShaderVariableFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShaderVariableFlags);

/// This describes the pipeline state shading rate flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALPipelineShadingRateFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None         = 0U,         ///< Shader variable has no special properties.
    PerPrimitive = XII_BIT(0), ///< This indicates that the pipeline state will be used with per draw or per primitive shading rate.
    TextureBased = XII_BIT(1), ///< This indicates that the pipeline state will be used with texture-based shading rate.

    Default = None
  };

  struct Bits
  {
    StorageType PerPrimitive : 1;
    StorageType TextureBased : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALPipelineShadingRateFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALPipelineShadingRateFlags);

/// This describes the shader variable property flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALPipelineType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Graphics,   ///< Graphics pipeline.
    Compute,    ///< Compute pipeline.
    Mesh,       ///< Mesh pipeline.
    RayTracing, ///< Ray tracing pipeline.
    Tile,       ///< Tile pipeline.

    ENUM_COUNT,

    Invalid = 0xFF,

    Default = Invalid
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALPipelineType);

/// This describes the sample information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSampleDescription : public xiiHashableStruct<xiiGALSampleDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt8 m_uiCount   = 1U; ///< The sample count.
  xiiUInt8 m_uiQuality = 0U; ///< The sample quality.
};

/// This describes the shader variable information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderResourceVariableDescription : public xiiHashableStruct<xiiGALShaderResourceVariableDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiStringView                          m_sName;                                          ///< The shader variable name.
  xiiBitflags<xiiGALShaderType>          m_ShaderStages = xiiGALShaderType::Unknown;       ///< The shader stages this resources variable applies to. If more than one shader stage is specified, the variable will be shared between these stages. Shader stages used by different variables with the same name must not overlap.
  xiiBitflags<xiiGALShaderVariableFlags> m_Flags        = xiiGALShaderVariableFlags::None; ///< The shader variable flags.
};

/// This describes graphics pipeline information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsPipelineDescription
{
  XII_ALWAYS_INLINE bool operator==(const xiiGALGraphicsPipelineDescription& rhs) const = default;

  xiiSharedPtr<xiiGALBlendState>              m_pBlendState;                                               ///< The reference-counted pointer to the blend state object to be used with the pipeline.
  xiiSharedPtr<xiiGALRasterizerState>         m_pRasterizerState;                                          ///< The reference-counted pointer to the rasterizer state object to be used with the pipeline.
  xiiSharedPtr<xiiGALDepthStencilState>       m_pDepthStencilState;                                        ///< The reference-counted pointer to the depth-stencil state object to be used with the pipeline.
  xiiSharedPtr<xiiGALInputLayout>             m_pInputLayout;                                              ///< The reference-counted pointer to the vertex input layout, ignored in a mesh pipeline.
  xiiSharedPtr<xiiGALRenderPass>              m_pRenderPass;                                               ///< The reference-counted pointer to the render pass object to be used with the pipeline.
  xiiUInt32                                   m_uiSampleMask      = 0xFFFFFFFFU;                           ///< A 32-bit sample mask that determines which samples get updated in all the active render targets. A sample mask is always applied; it is independent of whether multisampling is enabled, and does not depend on whether an application uses multisample render targets.
  xiiEnum<xiiGALPrimitiveTopology>            m_PrimitiveTopology = xiiGALPrimitiveTopology::TriangleList; ///< The primitive topology type, ignored in a mesh pipeline. The default is xiiGALPrimitiveTopology::TriangleList.
  xiiUInt8                                    m_uiViewportCount   = 1U;                                    ///< The number of viewports used by this pipeline. The default is 1.
  xiiUInt8                                    m_uiSubpassIndex    = 0U;                                    ///< The subpass index within the render pass. The default is 0.
  xiiBitflags<xiiGALPipelineShadingRateFlags> m_ShadingRateFlags  = xiiGALPipelineShadingRateFlags::None;  ///< Shading rate flags that specify which type of the shading rate will be used with this pipeline. The default is xiiGALPipelineShadingRateFlags::None.
  xiiGALSampleDescription                     m_SampleDescription;                                         ///< Multi-sampling parameters.
};

/// This describes the ray tracing general shader group information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingGeneralShaderGroupDescription
{
  XII_ALWAYS_INLINE bool operator==(const xiiGALRayTracingGeneralShaderGroupDescription& rhs) const = default;

  xiiHashedString            m_sName;   ///< The unique group name.
  xiiSharedPtr<xiiGALShader> m_pShader; ///< The reference-counted pointer to the shader of type xiiGALShaderType::RayGeneration, xiiGALShaderType::RayMiss, or xiiGALShaderType::Callable. This must not be invalid.
};

/// This describes the ray tracing general shader group information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingTriangleHitShaderGroupDescription
{
  XII_ALWAYS_INLINE bool operator==(const xiiGALRayTracingTriangleHitShaderGroupDescription& rhs) const = default;

  xiiHashedString            m_sName;             ///< The unique group name.
  xiiSharedPtr<xiiGALShader> m_pClosestHitShader; ///< The reference-counted pointer to the shader of type xiiGALShaderType::RayClosestHit. This must not be invalid.
  xiiSharedPtr<xiiGALShader> m_pAnyHitShader;     ///< The reference-counted pointer to the shader of type xiiGALShaderType::RayAnyHit. This can be invalid.
};

/// This describes the ray tracing general shader group information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingProceduralHitShaderGroupDescription
{
  XII_ALWAYS_INLINE bool operator==(const xiiGALRayTracingProceduralHitShaderGroupDescription& rhs) const = default;

  xiiHashedString            m_sName;               ///< The unique group name.
  xiiSharedPtr<xiiGALShader> m_pIntersectionShader; ///< The reference-counted pointer to the shader of type xiiGALShaderType::RayIntersection. This must not be invalid.
  xiiSharedPtr<xiiGALShader> m_pClosestHitShader;   ///< The reference-counted pointer to the shader of type xiiGALShaderType::RayClosestHit. This can be invalid.
  xiiSharedPtr<xiiGALShader> m_pAnyHitShader;       ///< The reference-counted pointer to the shader of type xiiGALShaderType::RayAnyHit. This can be invalid.
};

/// This describes the ray tracing pipeline information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingPipelineDescription : public xiiHashableStruct<xiiGALRayTracingPipelineDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt16 m_uiShaderRecordSize  = 0U; ///< Size of the additional data passed to the shader. Shader record size plus shader group size (32 bytes) must be aligned to 32 bytes. Shader record size plus shader group size (32 bytes) must not exceed 4096 bytes
  xiiUInt8  m_uiMaxRecursionDepth = 0U; ///< Number of recursive calls of TraceRay() in HLSL. Zero means no tracing of rays at all, only ray-gen shader will be executed. See Device MaxRayTracingRecursionDepth.
};

/// This describes the tile pipeline information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTilePipelineDescription
{
  XII_ALWAYS_INLINE bool operator==(const xiiGALTilePipelineDescription& rhs) const = default;

  xiiEnum<xiiGALSampleCount>                        m_SampleCount = xiiGALSampleCount::OneSample; ///< The number of samples in the render targets.
  xiiHybridArray<xiiEnum<xiiGALResourceFormat>, 2U> m_RenderTargetFormats;                        ///< The render target formats.
};

/// This describes the pipeline state creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALPipelineStateCreationDescription
{
  xiiEnum<xiiGALPipelineType>                   m_PipelineType;               ///< The pipeline type. The default is xiiGALPipelineType::Graphics.
  xiiSharedPtr<xiiGALPipelineResourceSignature> m_pPipelineResourceSignature; ///< The reference-counted pointer to the pipeline resource signature that contains the shader resource description.

  XII_ALWAYS_INLINE bool operator==(const xiiGALPipelineStateCreationDescription& rhs) const = default;

  /// Returns true if this pipeline state is a graphics pipeline.
  XII_ALWAYS_INLINE constexpr bool IsAnyGraphicsPipeline() const { return m_PipelineType == xiiGALPipelineType::Graphics || m_PipelineType == xiiGALPipelineType::Mesh; }

  /// Returns true if this pipeline state is a compute pipeline.
  XII_ALWAYS_INLINE constexpr bool IsComputePipeline() const { return m_PipelineType == xiiGALPipelineType::Compute; }

  /// Returns true if this pipeline state is a ray tracing pipeline.
  XII_ALWAYS_INLINE constexpr bool IsRayTracingPipeline() const { return m_PipelineType == xiiGALPipelineType::RayTracing; }

  /// Returns true if this pipeline state is a tile pipeline.
  XII_ALWAYS_INLINE constexpr bool IsTilePipeline() const { return m_PipelineType == xiiGALPipelineType::Tile; }
};

/// This describes the graphics pipeline state creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsPipelineStateCreationDescription : public xiiGALPipelineStateCreationDescription
{
  xiiGALGraphicsPipelineStateCreationDescription() noexcept
  {
    m_PipelineType = xiiGALPipelineType::Graphics;
  }

  XII_ALWAYS_INLINE bool operator==(const xiiGALGraphicsPipelineStateCreationDescription& rhs) const = default;

  xiiGALGraphicsPipelineDescription m_GraphicsPipeline;     ///< The graphics pipeline state description, see xiiGALGraphicsPipelineDescription.
  xiiSharedPtr<xiiGALShader>        m_pVertexShader;        ///< The reference-counted pointer to the vertex shader to be used with the pipeline.
  xiiSharedPtr<xiiGALShader>        m_pPixelShader;         ///< The reference-counted pointer to the pixel shader to be used with the pipeline.
  xiiSharedPtr<xiiGALShader>        m_pDomainShader;        ///< The reference-counted pointer to the domain shader to be used with the pipeline.
  xiiSharedPtr<xiiGALShader>        m_pHullShader;          ///< The reference-counted pointer to the hull shader to be used with the pipeline.
  xiiSharedPtr<xiiGALShader>        m_pGeometryShader;      ///< The reference-counted pointer to the geometry shader to be used with the pipeline.
  xiiSharedPtr<xiiGALShader>        m_pAmplificationShader; ///< The reference-counted pointer to the amplification shader to be used with the pipeline.
  xiiSharedPtr<xiiGALShader>        m_pMeshShader;          ///< The reference-counted pointer to the mesh shader to be used with the pipeline.
};

/// This describes the compute pipeline state creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALComputePipelineStateCreationDescription : public xiiGALPipelineStateCreationDescription
{
  xiiGALComputePipelineStateCreationDescription() noexcept
  {
    m_PipelineType = xiiGALPipelineType::Compute;
  }

  XII_ALWAYS_INLINE bool operator==(const xiiGALComputePipelineStateCreationDescription& rhs) const = default;

  xiiSharedPtr<xiiGALShader> m_pComputeShader; ///< The reference-counted pointer to the compute shader to be used with the pipeline.
};

/// This describes the mesh pipeline state creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingPipelineStateCreationDescription : public xiiGALPipelineStateCreationDescription
{
  xiiGALRayTracingPipelineStateCreationDescription() noexcept
  {
    m_PipelineType = xiiGALPipelineType::RayTracing;
  }

  XII_ALWAYS_INLINE bool operator==(const xiiGALRayTracingPipelineStateCreationDescription& rhs) const = default;

  xiiGALRayTracingPipelineDescription                                  m_RayTracingPipeline;          ///< The ray tracing pipeline state description, see xiiGALRayTracingPipelineDescription.
  xiiHashedString                                                      m_sShaderRecordName;           ///< In Direct3D12, the name of the constant buffer that will be used by the local root signature. This is unused if m_uiShaderRecordSize is zero.
  xiiDynamicArray<xiiGALRayTracingGeneralShaderGroupDescription>       m_GeneralShaders;              ///< An array of xiiGALRayTracingGeneralShaderGroupDescription structures that contain the shader group description.
  xiiDynamicArray<xiiGALRayTracingTriangleHitShaderGroupDescription>   m_TriangleHitShaders;          ///< An array of xiiGALRayTracingTriangleHitShaderGroupDescription structures that contain the shader group description.
  xiiDynamicArray<xiiGALRayTracingProceduralHitShaderGroupDescription> m_ProceduralHitShaders;        ///< An array of xiiGALRayTracingProceduralHitShaderGroupDescription structures that contain the shader group description.
  xiiUInt32                                                            m_uiMaximumAttributeSize = 0U; ///< In Direct3D12, the maximum hit shader attribute size in bytes. If zero then maximum allowed size will be used.
  xiiUInt32                                                            m_uiMaximumPayloadSize   = 0U; ///< In Direct3D12, the maximum payload size in bytes. If zero then maximum allowed size will be used.
};

/// This describes the mesh pipeline state creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTilePipelineStateCreationDescription : public xiiGALPipelineStateCreationDescription
{
  xiiGALTilePipelineStateCreationDescription() noexcept
  {
    m_PipelineType = xiiGALPipelineType::Tile;
  }

  XII_ALWAYS_INLINE bool operator==(const xiiGALTilePipelineStateCreationDescription& rhs) const = default;

  xiiGALTilePipelineDescription m_TilePipeline; ///< The tile pipeline state description, see xiiGALTilePipelineDescription.
  xiiSharedPtr<xiiGALShader>    m_pTileShader;  ///< The reference-counted pointer to the tile shader to be used with the pipeline.
};

/// Interface that defines methods to manipulate a pipeline state object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALPipelineState : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALPipelineState, xiiGALDeviceObject);

public:
  /// This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALPipelineStateCreationDescription& GetDescription() const { return m_Description; };

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALPipelineState(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALPipelineState();

protected:
  xiiGALPipelineStateCreationDescription m_Description;
};

/// Interface that defines methods to manipulate a graphics pipeline state object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsPipelineState : public xiiGALPipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALGraphicsPipelineState, xiiGALPipelineState);

public:
  /// This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALGraphicsPipelineStateCreationDescription& GetDescription() const { return m_Description; };

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALGraphicsPipelineState(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALGraphicsPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALGraphicsPipelineState();

  virtual xiiResult InitPlatform() = 0;

protected:
  xiiGALGraphicsPipelineStateCreationDescription m_Description;
};

/// Interface that defines methods to manipulate a compute pipeline state object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALComputePipelineState : public xiiGALPipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALComputePipelineState, xiiGALPipelineState);

public:
  /// This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALComputePipelineStateCreationDescription& GetDescription() const { return m_Description; };

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALComputePipelineState(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALComputePipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALComputePipelineState();

  virtual xiiResult InitPlatform() = 0;

protected:
  xiiGALComputePipelineStateCreationDescription m_Description;
};

/// Interface that defines methods to manipulate a ray tracing pipeline state object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingPipelineState : public xiiGALPipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALRayTracingPipelineState, xiiGALPipelineState);

public:
  /// This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALRayTracingPipelineStateCreationDescription& GetDescription() const { return m_Description; };

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALRayTracingPipelineState(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALRayTracingPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALRayTracingPipelineState();

  virtual xiiResult InitPlatform() = 0;

protected:
  xiiGALRayTracingPipelineStateCreationDescription m_Description;
};

/// Interface that defines methods to manipulate a tile pipeline state object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALTilePipelineState : public xiiGALPipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALTilePipelineState, xiiGALPipelineState);

public:
  /// This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALTilePipelineStateCreationDescription& GetDescription() const { return m_Description; };

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALTilePipelineState(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALTilePipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALTilePipelineState();

  virtual xiiResult InitPlatform() = 0;

protected:
  xiiGALTilePipelineStateCreationDescription m_Description;
};
