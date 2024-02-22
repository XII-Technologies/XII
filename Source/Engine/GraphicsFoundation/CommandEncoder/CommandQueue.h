#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/DeviceObject.h>

/// \brief Interface that defines methods to manipulate a command queue object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandQueue : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandQueue, xiiGALDeviceObject);

public:
  /// \brief This returns the value of the internal fence that will be signaled the next time.
  virtual xiiUInt64 GetNextFenceValue() const = 0;

  /// \brief This returns the last completed value of the internal fence.
  virtual xiiUInt64 GetCompletedFenceValue() const = 0;

  /// \brief This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() = 0;

protected:
  friend class xiiGALDevice;

  xiiGALCommandQueue();

  virtual ~xiiGALCommandQueue();
};

#include <GraphicsFoundation/CommandEncoder/Implementation/CommandQueue_inl.h>
