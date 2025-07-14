#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <GraphicsFoundation/Declarations/Object.h>

/// \brief This describes the fence creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandQueueCreationDescription : public xiiHashableStruct<xiiGALCommandQueueCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  xiiBitflags<xiiGALCommandQueueFlags> m_QueueFlags = xiiGALCommandQueueFlags::None; ///< The command queue flags.
};

/// \brief Interface that defines methods to manipulate a command queue object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandQueue : public xiiGALObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandQueue, xiiGALObject);

public:
  /// \brief This returns the creation description for this object.
  [[nodiscard]] XII_ALWAYS_INLINE const xiiGALCommandQueueCreationDescription& GetDescription() const { return m_Description; };

  /// \brief This returns the device for this object.
  [[nodiscard]] XII_ALWAYS_INLINE xiiGALDevice* GetDevice() const { return m_pDevice; };

  /// \brief This returns the value of the internal fence that will be signaled the next time.
  virtual xiiUInt64 GetNextFenceValue() const = 0;

  /// \brief This returns the last completed value of the internal fence.
  virtual xiiUInt64 GetCompletedFenceValue() = 0;

  /// \brief This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() = 0;

  /// \brief This begins a command list for recording commands.
  [[nodiscard]] virtual xiiSharedPtr<xiiGALCommandList> BeginCommandList() = 0;

protected:
  friend class xiiGALDevice;

  xiiGALCommandQueue(xiiGALDevice* pDevice, const xiiGALCommandQueueCreationDescription& creationDescription);

  virtual ~xiiGALCommandQueue();

protected:
  xiiGALCommandQueueCreationDescription m_Description;

  xiiGALDevice* m_pDevice;
};
