#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/DeviceObject.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief Base class for all GAL resources (textures, buffers, etc).
class XII_GRAPHICSFOUNDATION_DLL xiiGALResource : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALResource, xiiGALDeviceObject);

public:
  /// \brief This sets the buffer usage state.
  ///
  /// \note This method does not perform state transition, but resets the buffer state to the given value.
  ///       This method should be used after manually managing the buffer state to hand over state management back to the engine.
  virtual void SetResourceState(xiiBitflags<xiiGALResourceStateFlags> stateFlags);

  /// \brief This returns the buffer state.
  XII_NODISCARD virtual xiiBitflags<xiiGALResourceStateFlags> GetResourceState() const;

protected:
  friend class xiiGALDevice;

  xiiGALResource(xiiGALDevice* pDevice);

  xiiBitflags<xiiGALResourceStateFlags> m_ResourceState;

  xiiHashTable<xiiUInt32, xiiGALBufferViewHandle>  m_BufferViews;
  xiiHashTable<xiiUInt32, xiiGALTextureViewHandle> m_TextureViews;

  xiiGALBufferViewHandle  m_DefaultBufferViews[xiiGALBufferViewType::ENUM_COUNT];
  xiiGALTextureViewHandle m_DefaultTextureViews[xiiGALTextureViewType::ENUM_COUNT];
};

/// \brief Base class for all GAL resource views (texture and buffer views).
class XII_GRAPHICSFOUNDATION_DLL xiiGALResourceView : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALResourceView, xiiGALDeviceObject);

public:
protected:
  friend class xiiGALDevice;

  xiiGALResourceView(xiiGALDevice* pDevice);
};

#include <GraphicsFoundation/Resources/Implementation/Resource_inl.h>
