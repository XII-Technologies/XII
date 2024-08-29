#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/Object.h>

/// \brief Base Graphics Abstraction Layer Object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceObject : public xiiGALObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDeviceObject, xiiGALObject);

public:
  /// \brief Returns the xiiGALDevice that created this resource.
  ///
  /// \note This does **not** increase the ref count on the device.
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALDevice* GetDevice() const { return m_pDevice; };

protected:
  friend class xiiGALDevice;

  xiiGALDeviceObject(xiiGALDevice* pDevice);

protected:
  xiiGALDevice* m_pDevice;
};
