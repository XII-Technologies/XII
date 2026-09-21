/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/DeviceObject.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// Base class for all GAL resources (textures, buffers, etc).
class XII_GRAPHICSFOUNDATION_DLL xiiGALResource : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALResource, xiiGALDeviceObject);

public:
  /// This sets the buffer usage state.
  ///
  /// \note This method does not perform state transition, but resets the buffer state to the given value.
  ///       This method should be used after manually managing the buffer state to hand over state management back to the engine.
  virtual void SetResourceState(xiiBitflags<xiiGALResourceStateFlags> stateFlags) { m_ResourceState = stateFlags; }

  /// This returns the buffer state.
  [[nodiscard]] XII_ALWAYS_INLINE virtual xiiBitflags<xiiGALResourceStateFlags> GetResourceState() const { return m_ResourceState; }

  /// This returns true if the resource is in known state by the engine.
  [[nodiscard]] XII_ALWAYS_INLINE bool IsInKnownState() const { return m_ResourceState != xiiGALResourceStateFlags::Unknown; }

  /// This returns true if the given resource state is set.
  [[nodiscard]] XII_ALWAYS_INLINE bool CheckState(xiiBitflags<xiiGALResourceStateFlags> resourceState) const
  {
    XII_ASSERT_DEV(IsInKnownState(), "Resource state is unknown.");
    return m_ResourceState.AreAllSet(resourceState);
  }

  /// This returns true if any of the given resource state are set.
  [[nodiscard]] XII_ALWAYS_INLINE bool CheckAnyState(xiiBitflags<xiiGALResourceStateFlags> resourceState) const
  {
    XII_ASSERT_DEV(IsInKnownState(), "Resource state is unknown.");
    return m_ResourceState.IsAnySet(resourceState);
  }

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALResource(xiiSharedPtr<xiiGALDevice> pDevice);
  virtual ~xiiGALResource();

  xiiBitflags<xiiGALResourceStateFlags> m_ResourceState;
};

/// Base class for all GAL resource views (texture and buffer views).
class XII_GRAPHICSFOUNDATION_DLL xiiGALResourceView : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALResourceView, xiiGALDeviceObject);

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALResourceView(xiiSharedPtr<xiiGALDevice> pDevice);
  virtual ~xiiGALResourceView();
};
