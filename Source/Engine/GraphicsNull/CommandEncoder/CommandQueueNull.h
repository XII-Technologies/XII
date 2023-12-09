#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

class XII_GRAPHICSNULL_DLL xiiGALCommandQueueNull final : public xiiGALCommandQueue
{
public:
  /// \brief This returns the value of the internal fence that will be signaled the next time.
  virtual xiiUInt64 GetNextFenceValue() const override final;

  /// \brief This returns the last completed value of the internal fence.
  virtual xiiUInt64 GetCompletedFenceValue() const override final;

  /// \brief This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() override final;

protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALCommandQueueNull(const xiiGALCommandQueueCreationDescription& creationDescription);

  virtual ~xiiGALCommandQueueNull();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;
};

#include <GraphicsNull/CommandEncoder/Implementation/CommandQueueNull_inl.h>
