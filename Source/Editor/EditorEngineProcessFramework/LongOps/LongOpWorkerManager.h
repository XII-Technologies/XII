/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOpManager.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Utilities/Progress.h>

class xiiLongOpWorker;
struct xiiProgressEvent;
using xiiDataBuffer = xiiDynamicArray<xiiUInt8>;

/// The LongOp worker manager is active in the engine process of the editor.
///
/// This class has no public functionality, it communicates with the xiiLongOpControllerManager
/// and executes the xiiLongOpWorker's that are named by the respective xiiLongOpProxy's.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiLongOpWorkerManager final : public xiiLongOpManager
{
  XII_DECLARE_SINGLETON(xiiLongOpWorkerManager);

public:
  xiiLongOpWorkerManager();
  ~xiiLongOpWorkerManager();

private:
  friend class xiiLongOpTask;

  struct WorkerOpInfo
  {
    xiiUniquePtr<xiiLongOpWorker>                   m_pWorkerOp;
    xiiTaskGroupID                                  m_TaskID;
    xiiUuid                                         m_DocumentGuid;
    xiiUuid                                         m_OperationGuid;
    xiiProgress                                     m_Progress;
    xiiEvent<const xiiProgressEvent&>::Unsubscriber m_ProgressSubscription;
  };

  virtual void  ProcessCommunicationChannelEventHandler(const xiiProcessCommunicationChannel::Event& e) override;
  WorkerOpInfo* GetOperation(const xiiUuid& opGuid) const;
  void          LaunchWorkerOperation(WorkerOpInfo& opInfo, xiiStreamReader& config);
  void          WorkerProgressBarEventHandler(const xiiProgressEvent& e);
  void          RemoveOperation(xiiUuid opGuid);
  void          SendProgress(WorkerOpInfo& opInfo);
  void          WorkerOperationFinished(xiiUuid operationGuid, xiiResult result, xiiDataBuffer&& resultData);

  xiiDynamicArray<xiiUniquePtr<WorkerOpInfo>> m_WorkerOps;
};
