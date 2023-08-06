#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief This describes the buffer format.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBufferFormat : public xiiHashableStruct<xiiGALBufferFormat>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALValueType> m_ValueType;             ///< Type of components. For formatted buffer views, this value cannot be Undefined;
  xiiUInt8                 m_uiComponents  = 0U;    ///< Number of components. Allowed values: 1, 2, 3, 4.
  bool                     m_bIsNormalized = false; ///< For signed and unsigned integer value types indicates if the value should be normalized to [-1,+1] or [0, 1] range respectively. For floating point types, this member is ignored.
};

/// \brief This describes the buffer view creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBufferViewCreationDescription : public xiiHashableStruct<xiiGALBufferViewCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALBufferViewType> m_ViewType;          ///< The view type.
  xiiGALBufferFormat            m_Format;            ///< The format of the view. This member is only used for formatted and raw buffers. To create raw view of a raw buffer, set Format.ValueType member to Undefined (default value).
  xiiUInt64                     m_uiByteOffset = 0U; ///< The offset in bytes from the beginning of the buffer to the start of the buffer region referenced by the view.
  xiiUInt64                     m_uiByteWidth  = 0U; ///< The size in bytes of the referenced buffer region.
};

#include <GraphicsFoundation/Resources/Implementation/BufferView_inl.h>
