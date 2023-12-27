#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/DeviceObject.h>

/// \brief Base class for all GAL resources (textures, buffers, etc).
class XII_GRAPHICSFOUNDATION_DLL xiiGALResource : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALResource, xiiGALDeviceObject);

public:
protected:
  friend class xiiGALDevice;
};

#include <GraphicsFoundation/Resources/Implementation/Resource_inl.h>
