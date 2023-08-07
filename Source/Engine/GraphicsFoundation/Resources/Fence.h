#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

/// \brief This describes the fence type.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALFenceType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    CpuWaitOnly = 0U, ///< A basic fence that may be used for signaling the fence from GPU, and waiting for the fence on CPU.
    General,          ///< A general fence that may be used for signaling the fence from GPU, waiting for the fence on CPU, and waiting for the fence on GPU. If the native fence feature is enabled, the fence may also be used for signaling the fence on CPU, and waiting on GPU for a value that will be enqueued for signal later.

    ENUM_COUNT,

    Default = CpuWaitOnly
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALFenceType);

/// \brief This describes the fence creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALFenceCreationDescription : public xiiHashableStruct<xiiGALFenceCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALFenceType> m_Type; ///< The fence type.
};

#include <GraphicsFoundation/Resources/Implementation/Fence_inl.h>
