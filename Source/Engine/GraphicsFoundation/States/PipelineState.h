#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <GraphicsFoundation/Shader/Shader.h>

/// \brief This describes the shader variable property flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderVariableFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None             = 0U,         ///< Shader variable has no special properties.
    NoDynamicBuffers = XII_BIT(0), ///< This indicates that dynamic buffers will never be bound to the resource variable. This applies to uniform (constant) buffers, unordered access views, and shader resource views.
                                   ///
                                   ///  \remarks This flag directly translates to the xiiGALPipelineResourceFlag::NoDynamicBuffers in the internal pipeline resource signature.

    ENUM_COUNT = 2U,

    Default = None
  };

  struct Bits
  {
    StorageType NoDynamicBuffers : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALShaderVariableFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShaderVariableFlags);

/// \brief This describes the sample information.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALSampleDescription : public xiiHashableStruct<xiiGALSampleDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiUInt8 m_uiCount   = 1U; ///< The sample count.
  xiiUInt8 m_uiQuality = 0U; ///< The sample quality.
};

#include <GraphicsFoundation/States/Implementation/PipelineState_inl.h>
