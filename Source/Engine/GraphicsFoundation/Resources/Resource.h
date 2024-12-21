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
  virtual void SetResourceState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) {}

  /// \brief This returns the buffer state.
  [[nodiscard]] XII_ALWAYS_INLINE virtual xiiBitflags<xiiGALResourceStateFlags> GetResourceState() const { return m_ResourceState; }

  /// \brief This returns true if the resource is in known state by the engine.
  [[nodiscard]] XII_ALWAYS_INLINE virtual bool IsInKnownState() const { return m_ResourceState != xiiGALResourceStateFlags::Unknown; }

protected:
  friend class xiiGALDevice;

  xiiGALResource(xiiGALDevice* pDevice);

  xiiBitflags<xiiGALResourceStateFlags> m_ResourceState;
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
