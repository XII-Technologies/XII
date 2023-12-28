#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/DeviceObject.h>

/// \brief A command list interface. The command list has no methods. When a command list recording is finished, it is executed by the device context.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandList : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandList, xiiGALDeviceObject);

public:
protected:
  friend class xiiGALDevice;

  xiiGALCommandList();

  virtual ~xiiGALCommandList();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};

#include <GraphicsFoundation/CommandEncoder/Implementation/CommandList_inl.h>
