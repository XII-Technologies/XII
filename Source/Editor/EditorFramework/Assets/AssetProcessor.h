/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/FileSystem/DataDirPath.h>

#include <atomic>

struct xiiAssetCuratorEvent;
class xiiTask;
struct xiiAssetInfo;

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
    ProcessTaskStateChanged
  };

  Type m_Type;
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
    ReportResult
  };

public:
  xiiProcessTask();
  ~xiiProcessTask();

  xiiUInt32 m_uiProcessorID;

  bool Tick(bool bStartNewWork); // returns false, if all processing is done, otherwise call Tick again.

  bool IsConnected();

  bool HasProcessCrashed();

  xiiResult StartProcess();

  void ShutdownProcess();

private:
  void EventHandlerIPC(const xiiProcessCommunicationChannel::Event& e);

  bool GetNextAssetToProcess(xiiAssetInfo* pInfo, xiiUuid& out_guid, xiiDataDirPath& out_path);
  bool GetNextAssetToProcess(xiiUuid& out_guid, xiiDataDirPath& out_path);
  void OnProcessCrashed(xiiStringView message);

  State                                 m_State = State::LookingForWork;
  xiiUuid                               m_AssetGuid;
  xiiUInt64                             m_uiAssetHash   = 0;
  xiiUInt64                             m_uiThumbHash   = 0;
  xiiUInt64                             m_uiPackageHash = 0;
  xiiDataDirPath                        m_AssetPath;
  xiiEditorProcessCommunicationChannel* m_pIPC;
  bool                                  m_bProcessShouldBeRunning = false;
  xiiTransformStatus                    m_Status;
  xiiDynamicArray<xiiLogEntry>          m_LogEntries;
  xiiDynamicArray<xiiString>            m_TransitiveHull;
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

  void             StartProcessTask();
  void             StopProcessTask(bool bForce);
  ProcessTaskState GetProcessTaskState() const
  {
    return m_ProcessTaskState;
  }

  void AddLogWriter(xiiLoggingEvent::Handler handler);
  void RemoveLogWriter(xiiLoggingEvent::Handler handler);

public:
  // Can be called from worker threads!
  xiiEvent<const xiiAssetProcessorEvent&> m_Events;

private:
  friend class xiiProcessTask;
  friend class xiiProcessThread;
  friend class xiiAssetCurator;

  void Run();

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
};
