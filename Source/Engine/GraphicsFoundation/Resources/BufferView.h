#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/Resource.h>

/// \brief This describes the buffer format.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBufferFormat : public xiiHashableStruct<xiiGALBufferFormat>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALValueType> m_ValueType     = xiiGALValueType::Undefined; ///< Type of components. For formatted buffer views, this value cannot be Undefined.
  xiiUInt8                 m_uiComponents  = 0U;                         ///< Number of components. Allowed values: 1, 2, 3, 4.
  bool                     m_bIsNormalized = false;                      ///< For signed and unsigned integer value types indicates if the value should be normalized to [-1,+1] or [0, 1] range respectively. For floating point types, this member is ignored.
};

/// \brief This describes the buffer view creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBufferViewCreationDescription : public xiiHashableStruct<xiiGALBufferViewCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiStringView                 m_sName;                                      ///< Resource name. The default is an empty string view.
  xiiGALBufferHandle            m_hBuffer;                                    ///< The handle to the buffer of which the view is created.
  xiiEnum<xiiGALBufferViewType> m_ViewType = xiiGALBufferViewType::Undefined; ///< The view type. The default is Undefined.
  xiiGALBufferFormat            m_Format;                                     ///< The format of the view. This member is only used for formatted and raw buffers. To create raw view of a raw buffer, set Format.ValueType member to Undefined (default value).
  xiiUInt64                     m_uiByteOffset = 0U;                          ///< The offset in bytes from the beginning of the buffer to the start of the buffer region referenced by the view.
  xiiUInt64                     m_uiByteWidth  = 0U;                          ///< The size in bytes of the referenced buffer region.
};

/// \brief Interface that defines methods to manipulate a buffer view object.
///
/// \note The buffer view holds strong references to the buffer. The buffer will not be destroyed until all views are released.
class XII_GRAPHICSFOUNDATION_DLL xiiGALBufferView : public xiiGALResource<xiiGALBufferViewCreationDescription>
{
public:
  /// \brief Returns the resource of which the buffer view is created with.
  XII_ALWAYS_INLINE xiiGALResourceBase* GetResource() const { return m_pResource; }

protected:
  friend class xiiGALDevice;

  xiiGALBufferView(xiiGALResourceBase* pResource, const xiiGALBufferViewCreationDescription& creationDescription);

  virtual ~xiiGALBufferView();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;

  xiiGALResourceBase* m_pResource = nullptr;
};

#include <GraphicsFoundation/Resources/Implementation/BufferView_inl.h>
