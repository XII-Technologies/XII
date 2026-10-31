/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetProcessorMessages.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/FileSystem/DataDirPath.h>

#include <atomic>

struct xiiAssetCuratorEvent;
class xiiTask;
struct xiiAssetInfo;

/// One coalesced set of explicit requests, shared by all editor progress surfaces.
struct xiiAssetTransformProgress
{
  xiiUInt64 m_uiProjectGeneration = 0;
  xiiUInt32 m_uiTotal             = 0;
  xiiUInt32 m_uiCompleted         = 0;
  xiiUInt32 m_uiFailed            = 0;
  xiiUInt32 m_uiNeedsImport       = 0;
  xiiUInt32 m_uiCanceled          = 0;
  bool      m_bActive             = false;
  bool      m_bCancelRequested    = false;
  bool      m_bAllAssets          = false;
  xiiTime   m_Duration;
};

/// Log for all background processing results
class xiiAssetProcessorLog : public xiiLogInterface
{
public:
  virtual void HandleLogMessage(const xiiLoggingEventData& le) override;
  void         AddLogWriter(xiiLoggingEvent::Handler handler);
  void         RemoveLogWriter(xiiLoggingEvent::Handler handler);

  xiiLoggingEvent m_LoggingEvent;
};

struct xiiAssetProcessorEvent
{
  enum class Type
  {
    ProcessTaskStateChanged,
    AssetProcessed, ///< A background request reached its final result; not an arbitrary log message.
    TransformProgress,
    TransformsFinished
  };

  Type                      m_Type;
  xiiTransformResult::Enum  m_Result = xiiTransformResult::Success;
  xiiString                 m_sAssetPath; ///< Copied by listeners before returning to the processing thread.
  bool                      m_bRequested = false;
  bool                      m_bCanceled  = false;
  xiiAssetTransformProgress m_Progress;
};


class xiiProcessThread : public xiiThread
{
public:
  xiiProcessThread() :
    xiiThread("xiiProcessThread")
  {
  }

  virtual xiiUInt32 Run() override;
};

class xiiProcessTask
{
public:
  enum class State
  {
    LookingForWork,
    WaitingForConnection,
    Ready,
    Processing,
    WaitingForImport,
    ApplyingImport,
    ReportResult
  };

public:
  xiiProcessTask();
  ~xiiProcessTask();

  xiiUInt32 m_uiProcessorID;

  bool Tick(bool bStartNewWork); // returns false, if all processing is done, otherwise call Tick again.

  xiiResult StartProcess();

  void ShutdownProcess();

private:
  friend class xiiAssetProcessor;
  void EventHandlerIPC(const xiiProcessCommunicationChannel::Event& e);

  bool GetNextAssetToProcess(xiiAssetInfo* pInfo, xiiUuid& out_guid, xiiDataDirPath& out_path);
  bool GetNextAssetToProcess(xiiUuid& out_guid, xiiDataDirPath& out_path);
  void OnProcessCrashed(xiiStringView message);

  State                                 m_State = State::LookingForWork;
  xiiUuid                               m_AssetGuid;
  xiiProcessAssetMsg                    m_Request;
  xiiDataDirPath                        m_AssetPath;
  xiiEditorProcessCommunicationChannel* m_pIPC;
  xiiTransformStatus                    m_Status;
  xiiDynamicArray<xiiLogEntry>          m_LogEntries;
  bool                                  m_bRequested = false;
  xiiDynamicArray<xiiUuid>              m_ReservedAssets;
  // Protected by the curator mutex; consumed by the existing GUI-thread curator tick.
  bool      m_bImportPending = false;
  bool      m_bImportApplied = false;
  xiiString m_sImportBefore;
  xiiString m_sImportAfter;
};

/// Background asset processing is handled by this class.
/// Creates EditorProcessor processes.
class XII_EDITORFRAMEWORK_DLL xiiAssetProcessor
{
  XII_DECLARE_SINGLETON(xiiAssetProcessor);

public:
  enum class ProcessTaskState : xiiUInt8
  {
    Stopped,  ///< No EditorProcessor or the process thread is running.
    Running,  ///< Everything is active.
    Stopping, ///< Everything is still running but no new tasks are put into the EditorProcessors.
  };

  xiiAssetProcessor();
  ~xiiAssetProcessor();

  void StartProcessTask();
  bool IsAutomaticProcessingEnabled() const { return m_bAutomatic; }

  /// GUI-thread entry points. Save modified inputs before requesting; no widgets or documents
  /// are accessed by the process thread. Duplicate requests coalesce until their result arrives.
  xiiStatus QueueTransformAssets(xiiArrayPtr<const xiiUuid> assets, xiiBitflags<xiiTransformFlags> flags = xiiTransformFlags::TriggeredManually);
  /// Resolves all GUID/path references before queuing work. Invalid or ambiguous references fail the request.
  xiiStatus                 QueueTransformAssetReferences(xiiArrayPtr<const xiiString> references, xiiBitflags<xiiTransformFlags> flags = xiiTransformFlags::TriggeredManually);
  xiiStatus                 QueueTransformAllAssets();
  xiiAssetTransformProgress GetTransformProgress() const;
  /// Stops dispatching jobs. Force cancels running processes; wait joins the processor thread
  /// after outstanding jobs finish. Waiting must not hold the asset-curator mutex.
  void             StopProcessTask(bool bForce, bool bWait = false);
  ProcessTaskState GetProcessTaskState() const
  {
    return m_ProcessTaskState;
  }

  void AddLogWriter(xiiLoggingEvent::Handler handler);
  void RemoveLogWriter(xiiLoggingEvent::Handler handler);

  /// Captures a known asset's owner, hashes, active profile and all dependency categories for IPC.
  /// The output is unchanged on failure. The processor independently validates the transform state.
  /// The captured profile remains associated with the request if the editor later changes profiles.
  static xiiResult PrepareAssetRequest(xiiUuid assetGuid, xiiProcessAssetMsg& out_request, xiiStringView sProfile = {});

public:
  // Worker broadcasts and GUI listener teardown must be serialized. GUI listeners must queue their updates.
  xiiEvent<const xiiAssetProcessorEvent&, xiiMutex> m_Events;

private:
  friend class xiiProcessTask;
  friend class xiiProcessThread;
  friend class xiiAssetCurator;

  void      Run();
  void      StartProcessTask(bool bAutomatic);
  void      BroadcastTransformProgress(bool bFinished = false);
  void      ApplyImportResults();
  xiiStatus QueueTransforms(xiiArrayPtr<const xiiUuid> assets, xiiBitflags<xiiTransformFlags> flags, bool bAllAssets);

private:
  xiiAssetProcessorLog m_CuratorLog;

  // Process thread and its state
  xiiUniquePtr<xiiProcessThread> m_pThread;
  std::atomic<bool>              m_bForceStop = false; ///< If set, background processes will be killed when stopping without waiting for their current task to finish.

  // Locks writes to m_ProcessTaskState to make sure the state machine does not go from running to stopped before having fired stopping.
  mutable xiiMutex              m_ProcessorMutex;
  std::atomic<ProcessTaskState> m_ProcessTaskState = ProcessTaskState::Stopped;

  // Data owned by the process thread.
  xiiDynamicArray<xiiProcessTask> m_ProcessTasks;

  // Explicit requests and their aggregate are protected by the existing curator mutex.
  xiiDeque<xiiProcessAssetMsg> m_TransformRequests;
  xiiDeque<xiiProcessAssetMsg> m_FinalTransformRequests;
  xiiMap<xiiUuid, xiiUInt32>   m_RequestedAssets; ///< Pending and in-flight requests per asset.
  xiiSet<xiiUuid>              m_DeferredImports; ///< Live edits awaiting save/close; checked only by the GUI tick.
  xiiAssetTransformProgress    m_TransformProgress;
  xiiTime                      m_TransformStart;
  std::atomic<bool>            m_bAutomatic          = false;
  xiiUInt64                    m_uiProjectGeneration = 0;
};
