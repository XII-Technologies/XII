#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief This describes the command queue creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandQueueCreationDescription : public xiiHashableStruct<xiiGALCommandQueueCreationDescription>
{
  XII_DECLARE_POD_TYPE();
};

/// \brief Interface that defines methods to manipulate a command queue object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandQueue : public xiiGALObject<xiiGALCommandQueueCreationDescription>
{
public:
  /// \brief This returns the value of the internal fence that will be signaled the next time.
  virtual xiiUInt64 GetNextFenceValue() const = 0;

  /// \brief This returns the last completed value of the internal fence.
  virtual xiiUInt64 GetCompletedFenceValue() const = 0;

  /// \brief This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() = 0;

protected:
  friend class xiiGALDevice;

  xiiGALCommandQueue(const xiiGALCommandQueueCreationDescription& creationDescription);

  virtual ~xiiGALCommandQueue();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALCommandQueue);

#include <GraphicsFoundation/CommandEncoder/Implementation/CommandQueue_inl.h>
