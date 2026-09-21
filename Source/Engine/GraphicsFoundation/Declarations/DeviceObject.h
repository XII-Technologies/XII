/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Device/Device.h>

/// Base Graphics Abstraction Layer Object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceObject : public xiiGALObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDeviceObject, xiiGALObject);

public:
  /// Returns the xiiGALDevice that created this resource.
  ///
  /// \note This **increases** the ref count on the device.
  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALDevice> GetDevice() const { return m_pDevice; };

protected:
  friend class xiiGALDevice;
  friend class xiiMemoryUtils;

  xiiGALDeviceObject(xiiSharedPtr<xiiGALDevice> pDevice);
  virtual ~xiiGALDeviceObject();

protected:
  xiiSharedPtr<xiiGALDevice> m_pDevice;
};
