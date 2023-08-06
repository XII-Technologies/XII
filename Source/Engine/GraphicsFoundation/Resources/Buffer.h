#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief This describes the buffer access mode.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBufferMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Undefined = 0U, ///< Undefined buffer access mode.
    Formatted,      ///< Formatted buffer. Access to the buffer will use format conversion operations. In this mode, the element byte stride member of the buffer description defines the buffer element size. Buffer views can use different formats, but the format size must match the element byte stride.
    Structured,     ///< Structured buffer. In this mode, the element byte stride member of the buffer description defines the structure stride.
    Raw,            ///< Raw buffer. In this mode, the buffer is accessed as raw bytes. Formatted views of a raw buffer can also be created similar to formatted buffer. If formatted views are to be created, the element byte stride member of the buffer description must specify the size of the format.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALBufferMode);

/// \brief This describes the miscellaneous buffer flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALMiscBufferFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None        = 0U,         ///< No miscellaneous buffer flags.
    SparseAlias = XII_BIT(0), ///< For a sparse buffer, allow binding the same memory region in different buffer ranges or in different sparse buffers.

    ENUM_COUNT,

    Default = None
  };

  struct Bits
  {
    StorageType SparseAlias : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALMiscBufferFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALMiscBufferFlags);

/// \brief This describes the buffer creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBufferCreationDescription : public xiiHashableStruct<xiiGALBufferCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiStringView                      m_sName;
  xiiUInt64                          m_uiSize = 0U;                 ///< The size of the buffer in bytes. For a uniform (constant) buffer, this must be a multiple of 16.
  xiiBitflags<xiiGALBindFlags>       m_BindFlags;                   ///< The bind flags.
  xiiEnum<xiiGALResourceUsage>       m_ResourceUsage;               ///< The resource usage.
  xiiBitflags<xiiGALCPUAccessFlag>   m_CPUAccessFlags;              ///< The CPU access flags or None if no CPU access is allowed.
  xiiEnum<xiiGALBufferMode>          m_Mode;                        ///< The buffer mode.
  xiiBitflags<xiiGALMiscBufferFlags> m_MiscFlags;                   ///< The miscellaneous flags.
  xiiUInt32                          m_uiElementByteStride    = 0U; ///< The buffer element stride in bytes.
  xiiUInt64                          m_uiImmediateContextMask = 1U; ///< Indicates which immediate contexts are allowed to execute commands that use this buffer.
};

/// \brief This describes the buffer initial data.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBufferData : public xiiHashableStruct<xiiGALBufferData>
{
  XII_DECLARE_POD_TYPE();

  const void* m_pData      = nullptr; ///< The pointer to the data.
  xiiUInt64   m_uiDataSize = 0U;      ///< The data size in bytes.
};

#include <GraphicsFoundation/Resources/Implementation/Buffer_inl.h>
