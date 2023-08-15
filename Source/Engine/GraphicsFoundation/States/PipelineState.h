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
    InputAttachment = XII_BIT(1),  ///< This indicates that the resource is an input attachment in general layout, which allows simultaneously reading from the resource through the input attachment and writing to it via color or depth-stencil attachment.
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

#include <GraphicsFoundation/States/Implementation/PipelineState_inl.h>
