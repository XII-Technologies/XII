#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/DeviceObject.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief This describes the fence creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandQueueCreationDescription : public xiiHashableStruct<xiiGALCommandQueueCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiBitflags<xiiGALCommandQueueType> m_QueueType = xiiGALCommandQueueType::Unknown; ///< The command queue type that this command list uses.
};

/// \brief Interface that defines methods to manipulate a command queue object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandQueue : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandQueue, xiiGALDeviceObject);

public:
  /// \brief This returns the value of the internal fence that will be signaled the next time.
  virtual xiiUInt64 GetNextFenceValue() const = 0;

  /// \brief This returns the last completed value of the internal fence.
  virtual xiiUInt64 GetCompletedFenceValue() = 0;

  /// \brief This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() = 0;

  /// \brief This begins a command list for recording commmands.
  XII_NODISCARD virtual xiiGALCommandList* BeginCommandList() = 0;

protected:
  friend class xiiGALDevice;

  xiiGALCommandQueue(xiiGALDevice* pDevice, const xiiGALCommandQueueCreationDescription& creationDescription);

  virtual ~xiiGALCommandQueue();

  virtual xiiResult InitPlatform() = 0;

  virtual xiiResult DeInitPlatform() = 0;

protected:
  xiiGALCommandQueueCreationDescription m_Description;
};

#include <GraphicsFoundation/CommandEncoder/Implementation/CommandQueue_inl.h>
