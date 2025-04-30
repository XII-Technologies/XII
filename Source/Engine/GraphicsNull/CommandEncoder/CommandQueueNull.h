#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <Foundation/Types/UniquePtr.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

class XII_GRAPHICSNULL_DLL xiiGALCommandQueueNull final : public xiiGALCommandQueue
{
public:
  /// \brief This returns the value of the internal fence that will be signaled the next time.
  virtual xiiUInt64 GetNextFenceValue() const override final;

  /// \brief This returns the last completed value of the internal fence.
  virtual xiiUInt64 GetCompletedFenceValue() override final;

  /// \brief This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() override final;

  virtual xiiGALCommandList* BeginCommandList() override final;

protected:
  xiiUInt64 Submit(xiiGALCommandList* pCommandList);

protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;
  friend class xiiGALCommandListNull;

  xiiGALCommandQueueNull(xiiGALDeviceNull* pDeviceNull, const xiiGALCommandQueueCreationDescription& creationDescription);

  virtual ~xiiGALCommandQueueNull();

  xiiResult InitPlatform();

protected:
  xiiUniquePtr<xiiGALCommandListNull> m_pDefaultCommandList;
};
