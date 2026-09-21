/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/Resource.h>

/// This describes the buffer view creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBufferViewCreationDescription : public xiiHashableStruct<xiiGALBufferViewCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALBufferViewType> m_ViewType     = xiiGALBufferViewType::Undefined; ///< The view type. The default is Undefined.
  xiiEnum<xiiGALResourceFormat> m_Format       = xiiGALResourceFormat::Unknown;   ///< The format of the view. This member is only used for formatted and raw buffers. To create raw view of a raw buffer, set to xiiGALResourceFormat::Unknown. The default is xiiGALResourceFormat::Unknown.
  xiiUInt64                     m_uiByteOffset = 0U;                              ///< The offset in bytes from the beginning of the buffer to the start of the buffer region referenced by the view.
  xiiUInt64                     m_uiByteWidth  = 0U;                              ///< The size in bytes of the referenced buffer region.
};

/// Interface that defines methods to manipulate a buffer view object.
///
/// \note The buffer view holds strong references to the buffer. The buffer will not be destroyed until all views are released.
class XII_GRAPHICSFOUNDATION_DLL xiiGALBufferView : public xiiGALResourceView
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALBufferView, xiiGALResourceView);

public:
  /// This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALBufferViewCreationDescription& GetDescription() const { return m_Description; }

  /// Returns the buffer of which the buffer view is created with.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetBuffer() const { return m_pBuffer; }

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALBufferView(xiiSharedPtr<xiiGALDevice> pDevice, xiiSharedPtr<xiiGALBuffer> pBuffer, const xiiGALBufferViewCreationDescription& creationDescription);

  virtual ~xiiGALBufferView();

  virtual xiiResult InitPlatform() = 0;

protected:
  xiiSharedPtr<xiiGALBuffer> m_pBuffer;

  xiiGALBufferViewCreationDescription m_Description;
};
