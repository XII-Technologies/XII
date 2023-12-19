#pragma once

#include <EditorEngineProcessFramework/LongOps/Implementation/LongOpManager.h>

class xiiLongOpProxy;

/// \brief Events about all known long ops. Broadcast by xiiLongOpControllerManager.
struct xiiLongOpControllerEvent
{
  enum class Type
  {
    OpAdded,    ///< A new long op has been added / registered.
    OpRemoved,  ///< A long op has been deleted. The GUID is sent, but it cannot be resolved anymore.
    OpProgress, ///< The completion progress of a long op has changed.
  };

  Type    m_Type;
  xiiUuid m_OperationGuid; ///< Use xiiLongOpControllerManager::GetOperation() to resolve the GUID to the actual long op.
};

/// \brief The LongOp controller is active in the editor process and manages which long ops are available, running, etc.
///
/// All available long ops are registered with the controller, typically automatically by the xiiLongOpsAdapter,
/// although it is theoretically possible to register additional long ops.
///
/// Through the controller long ops can be started or canceled, which is exposed in the UI by the xiiQtLongOpsPanel.
///
/// Through the broadcast xiiLongOpControllerEvent, one can track the state of all long ops.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiLongOpControllerManager final : public xiiLongOpManager
{
  XII_DECLARE_SINGLETON(xiiLongOpControllerManager);

public:
  xiiLongOpControllerManager();
  ~xiiLongOpControllerManager();

  /// \brief Holds all information about the proxy long op on the editor side
  struct ProxyOpInfo
  {
    xiiUniquePtr<xiiLongOpProxy> m_pProxyOp;
    xiiUuid                      m_OperationGuid; ///< Identifies the operation itself.
    xiiUuid                      m_DocumentGuid;  ///< To which document the long op belongs. When the document is closed, all running long ops belonging to it
                                                  ///< will be canceled.
    xiiUuid m_ComponentGuid;                      ///< To which component in the scene document the long op is linked. If the component is deleted, the long op
                                                  ///< disappears as well.
    xiiTime m_StartOrDuration;                    ///< While m_bIsRunning is true, this is the time the long op started, once m_bIsRunning it holds the last
                                                  ///< duration of the long op execution.
    float m_fCompletion = 0.0f;                   ///< [0; 1] range for the progress.
    bool  m_bIsRunning  = false;                  ///< Whether the long op is currently being executed.
  };

  /// \brief Events about the state of all available long ops.
  xiiEvent<const xiiLongOpControllerEvent&> m_Events;

  /// \brief Typically called by xiiLongOpsAdapter when a component that has an xiiLongOpAttribute is added to a scene
  void RegisterLongOp(const xiiUuid& documentGuid, const xiiUuid& componentGuid, const char* szLongOpType);

  /// \brief Typically called by xiiLongOpsAdapter when a component that has an xiiLongOpAttribute is removed from a scene
  void UnregisterLongOp(const xiiUuid& documentGuid, const xiiUuid& componentGuid, const char* szLongOpType);

  /// \brief Starts executing the given long op. Typically called by the xiiQtLongOpsPanel.
  void StartOperation(xiiUuid opGuid);

  /// \brief Cancels a given long op. Typically called by the xiiQtLongOpsPanel.
  void CancelOperation(xiiUuid opGuid);

  /// \brief Cancels and deletes all operations linked to the given document. Makes sure to wait for all canceled ops.
  /// Typically called by the xiiLongOpsAdapter when a document is about to be closed.
  void CancelAndRemoveAllOpsForDocument(const xiiUuid& documentGuid);

  /// \brief Returns a pointer to the given long op, or null if the GUID does not exist.
  ProxyOpInfo* GetOperation(const xiiUuid& opGuid);

  /// \brief Gives access to all currently available long ops. Make sure the lock m_Mutex (of the xiiLongOpManager base class) while accessing this.
  const xiiDynamicArray<xiiUniquePtr<ProxyOpInfo>>& GetOperations() const { return m_ProxyOps; }

private:
  virtual void ProcessCommunicationChannelEventHandler(const xiiProcessCommunicationChannel::Event& e) override;

  void ReplicateToWorkerProcess(ProxyOpInfo& opInfo);
  void BroadcastProgress(ProxyOpInfo& opInfo);
  void RemoveOperation(xiiUuid opGuid);

  xiiDynamicArray<xiiUniquePtr<ProxyOpInfo>> m_ProxyOps;
};
