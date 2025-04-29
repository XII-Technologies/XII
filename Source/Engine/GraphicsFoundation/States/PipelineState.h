#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Shader/Shader.h>
#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/States/DepthStencilState.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>
#include <GraphicsFoundation/States/RasterizerState.h>

/// \brief This describes the shader variable property flags.
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

    ENUM_COUNT = 3U,

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

/// \brief This describes the pipeline state shading rate flags.
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

/// \brief This describes the shader variable property flags.
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

    Default = Graphics
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALPipelineType);

/// \brief This describes the sample information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSampleDescription : public xiiHashableStruct<xiiGALSampleDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt8 m_uiCount   = 1U; ///< The sample count.
  xiiUInt8 m_uiQuality = 0U; ///< The sample quality.
};

/// \brief This describes the shader variable information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderResourceVariableDescription : public xiiHashableStruct<xiiGALShaderResourceVariableDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiStringView                          m_sName;                                          ///< The shader variable name.
  xiiBitflags<xiiGALShaderType>          m_ShaderStages = xiiGALShaderType::Unknown;       ///< The shader stages this resources variable applies to. If more than one shader stage is specified, the variable will be shared between these stages. Shader stages used by different variables with the same name must not overlap.
  xiiBitflags<xiiGALShaderVariableFlags> m_Flags        = xiiGALShaderVariableFlags::None; ///< The shader variable flags.
};

/// \brief This describes graphics pipeline information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsPipelineDescription : public xiiHashableStruct<xiiGALGraphicsPipelineDescription>
{
  xiiSharedPtr<xiiGALShader>                  m_pVertexShader;                                             ///< The reference-counted pointer to the vertex shader to be used with the pipeline.
  xiiSharedPtr<xiiGALShader>                  m_pPixelShader;                                              ///< The reference-counted pointer to the pixel shader to be used with the pipeline.
  xiiSharedPtr<xiiGALShader>                  m_pDomainShader;                                             ///< The reference-counted pointer to the domain shader to be used with the pipeline.
  xiiSharedPtr<xiiGALShader>                  m_pHullShader;                                               ///< The reference-counted pointer to the hull shader to be used with the pipeline.
  xiiSharedPtr<xiiGALShader>                  m_pGeometryShader;                                           ///< The reference-counted pointer to the geometry shader to be used with the pipeline.
  xiiSharedPtr<xiiGALShader>                  m_pAmplificationShader;                                      ///< The reference-counted pointer to the amplification shader to be used with the pipeline.
  xiiSharedPtr<xiiGALShader>                  m_pMeshShader;                                               ///< The reference-counted pointer to the mesh shader to be used with the pipeline.
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

/// \brief This describes compute pipeline information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALComputePipelineDescription : public xiiHashableStruct<xiiGALComputePipelineDescription>
{
  xiiSharedPtr<xiiGALShader> m_pComputeShader; ///< The reference-counted pointer to the compute shader to be used with the pipeline.
};

/// \brief This describes the ray tracing general shader group information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingGeneralShaderGroupDescription : public xiiHashableStruct<xiiGALRayTracingGeneralShaderGroupDescription>
{
  xiiHashedString            m_sName;   ///< The unique group name.
  xiiSharedPtr<xiiGALShader> m_pShader; ///< The reference-counted pointer to the shader of type xiiGALShaderType::RayGeneration, xiiGALShaderType::RayMiss, or xiiGALShaderType::Callable. This must not be invalid.
};

/// \brief This describes the ray tracing general shader group information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingTriangleHitShaderGroupDescription : public xiiHashableStruct<xiiGALRayTracingTriangleHitShaderGroupDescription>
{
  xiiHashedString            m_sName;             ///< The unique group name.
  xiiSharedPtr<xiiGALShader> m_pClosestHitShader; ///< The reference-counted pointer to the shader of type xiiGALShaderType::RayClosestHit. This must not be invalid.
  xiiSharedPtr<xiiGALShader> m_pAnyHitShader;     ///< The reference-counted pointer to the shader of type xiiGALShaderType::RayAnyHit. This can be invalid.
};

/// \brief This describes the ray tracing general shader group information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingProceduralHitShaderGroupDescription : public xiiHashableStruct<xiiGALRayTracingProceduralHitShaderGroupDescription>
{
  xiiHashedString            m_sName;               ///< The unique group name.
  xiiSharedPtr<xiiGALShader> m_pIntersectionShader; ///< The reference-counted pointer to the shader of type xiiGALShaderType::RayIntersection. This must not be invalid.
  xiiSharedPtr<xiiGALShader> m_pClosestHitShader;   ///< The reference-counted pointer to the shader of type xiiGALShaderType::RayClosestHit. This can be invalid.
  xiiSharedPtr<xiiGALShader> m_pAnyHitShader;       ///< The reference-counted pointer to the shader of type xiiGALShaderType::RayAnyHit. This can be invalid.
};

/// \brief This describes the ray tracing pipeline information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingPipelineDescription
{
  xiiHashedString                                                      m_sShaderRecordName;           ///< In Direct3D12, the name of the constant buffer that will be used by the local root signature. This is unused if m_uiShaderRecordSize is zero.
  xiiDynamicArray<xiiGALRayTracingGeneralShaderGroupDescription>       m_GeneralShaders;              ///< An array of xiiGALRayTracingGeneralShaderGroupDescription structures that contain the shader group description.
  xiiDynamicArray<xiiGALRayTracingTriangleHitShaderGroupDescription>   m_TriangleHitShaders;          ///< An array of xiiGALRayTracingTriangleHitShaderGroupDescription structures that contain the shader group description.
  xiiDynamicArray<xiiGALRayTracingProceduralHitShaderGroupDescription> m_ProceduralHitShaders;        ///< An array of xiiGALRayTracingProceduralHitShaderGroupDescription structures that contain the shader group description.
  xiiUInt32                                                            m_uiMaximumAttributeSize = 0U; ///< In Direct3D12, the maximum hit shader attribute size in bytes. If zero then maximum allowed size will be used.
  xiiUInt32                                                            m_uiMaximumPayloadSize   = 0U; ///< In Direct3D12, the maximum payload size in bytes. If zero then maximum allowed size will be used.
  xiiUInt16                                                            m_uiShaderRecordSize     = 0U; ///< Size of the additional data passed to the shader. Shader record size plus shader group size (32 bytes) must be aligned to 32 bytes. Shader record size plus shader group size (32 bytes) must not exceed 4096 bytes
  xiiUInt8                                                             m_uiMaxRecursionDepth    = 0U; ///< Number of recursive calls of TraceRay() in HLSL. Zero means no tracing of rays at all, only ray-gen shader will be executed. See Device MaxRayTracingRecursionDepth.

  XII_ALWAYS_INLINE bool operator==(const xiiGALRayTracingPipelineDescription& rhs) const = default;
};

/// \brief This describes the tile pipeline information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALTilePipelineDescription : public xiiHashableStruct<xiiGALTilePipelineDescription>
{
  xiiEnum<xiiGALSampleCount>                                                    m_SampleCount = xiiGALSampleCount::OneSample; ///< The number of samples in the render targets.
  xiiStaticArray<xiiEnum<xiiGALResourceFormat>, XII_GAL_MAX_RENDERTARGET_COUNT> m_RenderTargetFormats;                        ///< The render target formats.
};

/// \brief This describes the pipeline state creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALPipelineStateCreationDescription
{
  xiiEnum<xiiGALPipelineType>                   m_PipelineType = xiiGALPipelineType::Graphics; ///< The pipeline type. The default is xiiGALPipelineType::Graphics.
  xiiSharedPtr<xiiGALPipelineResourceSignature> m_pPipelineResourceSignature;                  ///< The reference-counted pointer to the pipeline resource signature that contains the shader resource description.
  xiiGALGraphicsPipelineDescription             m_GraphicsPipeline;                            ///< The graphics pipeline description, see xiiGALGraphicsPipelineDescription.
  xiiGALComputePipelineDescription              m_ComputePipeline;                             ///< The compute pipeline description, see xiiGALComputePipelineDescription.
  xiiGALRayTracingPipelineDescription           m_RayTracingPipeline;                          ///< The ray tracing pipeline description, see xiiGALRayTracingPipelineDescription.
  xiiGALTilePipelineDescription                 m_TilePipeline;                                ///< The tile pipeline description, see xiiGALTilePipelineDescription.
  xiiUInt32                                     m_uiNodeMask = 0x0;                            ///< Node mask.

  XII_ALWAYS_INLINE bool operator==(const xiiGALPipelineStateCreationDescription& rhs) const = default;

  /// \brief Returns true if this pipeline state is a graphics pipeline.
  XII_ALWAYS_INLINE constexpr bool IsAnyGraphicsPipeline() const { return m_PipelineType == xiiGALPipelineType::Graphics || m_PipelineType == xiiGALPipelineType::Mesh; }

  /// \brief Returns true if this pipeline state is a compute pipeline.
  XII_ALWAYS_INLINE constexpr bool IsComputePipeline() const { return m_PipelineType == xiiGALPipelineType::Compute; }

  /// \brief Returns true if this pipeline state is a ray tracing pipeline.
  XII_ALWAYS_INLINE constexpr bool IsRayTracingPipeline() const { return m_PipelineType == xiiGALPipelineType::RayTracing; }

  /// \brief Returns true if this pipeline state is a tile pipeline.
  XII_ALWAYS_INLINE constexpr bool IsTilePipeline() const { return m_PipelineType == xiiGALPipelineType::Tile; }
};

/// \brief Interface that defines methods to manipulate a pipeline state object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALPipelineState : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALPipelineState, xiiGALDeviceObject);

public:
  /// \brief This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALPipelineStateCreationDescription& GetDescription() const { return m_Description; };

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALPipelineState(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALPipelineState();

  virtual xiiResult InitPlatform() = 0;

protected:
  xiiGALPipelineStateCreationDescription m_Description;
};
