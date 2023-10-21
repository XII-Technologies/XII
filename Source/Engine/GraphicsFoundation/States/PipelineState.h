#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <GraphicsFoundation/Shader/ShaderResourceVariable.h>

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

    ENUM_COUNT = 3U,

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

  xiiString                                 m_sName;                                                   ///< The shader variable name.
  xiiBitflags<xiiGALShaderStage>            m_ShaderStages = xiiGALShaderStage::Unknown;               ///< The shader stages this resources variable applies to. If more than one shader stage is specified, the variable will be shared between these stages. Shader stages used by different variables with the same name must not overlap.
  xiiEnum<xiiGALShaderResourceVariableType> m_Type         = xiiGALShaderResourceVariableType::Static; ///< The shader variable type.
  xiiBitflags<xiiGALShaderVariableFlags>    m_Flags        = xiiGALShaderVariableFlags::None;          ///< The shader variable flags.
};

/// \brief This describes graphics pipeline information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsPipelineDescription : public xiiHashableStruct<xiiGALGraphicsPipelineDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiGALBlendStateHandle                      m_hBlendState;                                                   ///< Handle to blend state description.
  xiiUInt32                                   m_uiSampleMask = 0xFFFFFFFF;                                     ///< 32-bit sample mask that determines which samples get updated in all the active render targets. A sample mask is always applied; it is independent of whether multisampling is enabled, and does not depend on whether an application uses multisample render targets.
  xiiGALRasterizerStateHandle                 m_hRasterizerState;                                              ///< Handle to rasterizer state description.
  xiiGALDepthStencilStateHandle               m_hDepthStencilState;                                            ///< Handle to depth-stencil state description.
  xiiGALInputLayoutHandle                     m_hInputLayout;                                                  ///< Handle to vertex input layout, ignored in a mesh pipeline.
  xiiEnum<xiiGALPrimitiveTopology>            m_PrimitiveTopology     = xiiGALPrimitiveTopology::TriangleList; ///< Primitive topology type, ignored in a mesh pipeline.
  xiiUInt8                                    m_ViewportCount         = 1U;                                    ///< The number of viewports used by this pipeline.
  xiiUInt8                                    m_uiSubpassIndex        = 0U;                                    ///< The subpass index within the render pass.
  xiiBitflags<xiiGALPipelineShadingRateFlags> m_ShadingRateFlags      = xiiGALPipelineShadingRateFlags::None;  ///< Shading rate flags that specify which type of the shading rate will be used with this pipeline.
  bool                                        m_bReadOnlyDepthStencil = false;                                 ///< Indicates that the pipeline will be used with read-only depth-stencil buffer.
  xiiGALSampleDescription                     m_SampleDescription;                                             ///< Multi-sampling parameters.
  xiiGALRenderPassHandle                      m_hRenderPass;                                                   ///< Handle to the render pass object.
};

/// \brief This describes the ray tracing general shader group information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingGeneralShaderGroupDescription : public xiiHashableStruct<xiiGALRayTracingGeneralShaderGroupDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiString          m_sName;   ///< Unique group name.
  xiiGALShaderHandle m_hShader; ///< Handle to the shader. The shader type must be of RayGeneration, RayMiss, or Callable.
};

/// \brief This describes the ray tracing triangle hit shader group information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingTriangleHitShaderGroupDescription : public xiiHashableStruct<xiiGALRayTracingTriangleHitShaderGroupDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiString          m_sName;             ///< Unique group name.
  xiiGALShaderHandle m_hClosestHitShader; ///< Handle to the closest hit shader.
  xiiGALShaderHandle m_hAnyHitShader;     ///< Handle to the any hit shader. This is optional.
};

/// \brief This describes the ray tracing procedural hit shader group information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingProceduralHitShaderGroupDescription : public xiiHashableStruct<xiiGALRayTracingProceduralHitShaderGroupDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiString          m_sName;               ///< Unique group name.
  xiiGALShaderHandle m_hIntersectionShader; ///< Handle to the closest hit shader.
  xiiGALShaderHandle m_hClosestHitShader;   ///< Handle to the closest hit shader. This is optional.
  xiiGALShaderHandle m_hAnyHitShader;       ///< Handle to the any hit shader. This is optional.
};

/// \brief This describes the ray tracing procedural hit shader group information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRayTracingPipelineDescription : public xiiHashableStruct<xiiGALRayTracingPipelineDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt16 m_uiShaderRecordSize  = 0U; ///< Size of the additional data passed to the shader. Shader record size plus shader group size (32 bytes) must be aligned to 32 bytes. Shader record size plus shader group size (32 bytes) must not exceed 4096 bytes
  xiiUInt8  m_uiMaxRecursionDepth = 0U; ///< Number of recursive calls of TraceRay() in HLSL. Zero means no tracing of rays at all, only ray-gen shader will be executed. See Device MaxRayTracingRecursionDepth.
};

/// \brief This describes the shader variable property flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALPipelineType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Graphics,
    Compute,
    Mesh,
    RayTracing,
    Tile,

    ENUM_COUNT,

    Invalid = 0xFF,

    Default = Graphics
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALPipelineType);

#include <GraphicsFoundation/States/Implementation/PipelineState_inl.h>
