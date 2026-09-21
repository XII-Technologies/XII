/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <GraphicsFoundation/Declarations/Object.h>

/// This describes the fence creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandQueueCreationDescription : public xiiHashableStruct<xiiGALCommandQueueCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiBitflags<xiiGALCommandQueueFlags> m_QueueFlags = xiiGALCommandQueueFlags::None; ///< The command queue flags.
};

/// Interface that defines methods to manipulate a command queue object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandQueue : public xiiGALObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandQueue, xiiGALObject);

public:
  /// This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALCommandQueueCreationDescription& GetDescription() const { return m_Description; };

  /// This returns the device for this object.
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALDevice* GetDevice() const { return m_pDevice; };

  /// This returns the value of the internal fence that will be signaled the next time.
  virtual xiiUInt64 GetNextFenceValue() const = 0;

  /// This returns the last completed value of the internal fence.
  virtual xiiUInt64 GetCompletedFenceValue() = 0;

  /// Submits a recorded command list to the GPU queue for execution.
  ///
  /// \note Command list must be compatible with the queue's flag configuration.
  /// Invalid or misconfigured lists may trigger assertions in development builds.
  ///
  /// \param pCommandList - The command list to be submitted. Must not be null. Its recording state must be 'Ended' or 'Recording'.
  ///
  /// \return The fence value that can be used to query GPU completion status for this submission.
  virtual xiiUInt64 Submit(xiiGALCommandList* pCommandList);

  /// This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() = 0;

protected:
  friend class xiiGALDevice;

  xiiGALCommandQueue(xiiGALDevice* pDevice, const xiiGALCommandQueueCreationDescription& creationDescription);

  virtual ~xiiGALCommandQueue();

  virtual xiiUInt64 SubmitPlatform(xiiGALCommandList* pCommandList) = 0;

protected:
  xiiGALCommandQueueCreationDescription m_Description;

  xiiGALDevice* m_pDevice;
};
