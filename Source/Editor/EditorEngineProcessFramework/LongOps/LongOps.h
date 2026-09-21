/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Foundation/Reflection/Reflection.h>

class xiiStringBuilder;
class xiiStreamWriter;
class xiiProgress;

//////////////////////////////////////////////////////////////////////////

/// Proxy long ops represent a long operation on the editor side.
///
/// Proxy long ops have little functionality other than naming which xiiLongOpWorker to execute
/// in the engine process and to feed it with the necessary parameters.
/// Since the proxy long op runs in the editor process, it may access xiiDocumentObject's
/// and extract data from them.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiLongOpProxy : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLongOpProxy, xiiReflectedClass);

public:
  /// Called once by xiiLongOpControllerManager::RegisterLongOp() to inform the proxy
  /// to which xiiDocument and component (xiiDocumentObject) it is linked.
  virtual void InitializeRegistered(const xiiUuid& documentGuid, const xiiUuid& componentGuid) {}

  /// Called by the xiiQtLongOpsPanel to determine the display string to be shown in the UI.
  virtual xiiStringView GetDisplayName() const = 0;

  /// Called every time the long op shall be executed
  /// \param out_sReplicationOpType must name the xiiLongOpWorker that shall be executed in the engine process.
  /// \param config can be optionally written to. The data is transmitted to the xiiLongOpWorker on the other side
  /// and fed to it in xiiLongOpWorker::InitializeExecution().
  virtual void GetReplicationInfo(xiiStringBuilder& out_sReplicationOpType, xiiStreamWriter& inout_config) = 0;

  /// Called once the corresponding xiiLongOpWorker has finished.
  /// \param result Whether the operation succeeded or failed (e.g. via user cancellation).
  /// \param resultData Optional data written by xiiLongOpWorker::Execute().
  virtual void Finalize(xiiResult result, const xiiDataBuffer& resultData) {}
};

//////////////////////////////////////////////////////////////////////////

/// Worker long ops are executed by the editor engine process.
///
/// They typically do the actual long processing. Since they run in the engine process, they have access
/// to the runtime scene graph and resources but not the editor representation of the scene.
///
/// xiiLongOpWorker instances are automatically instantiated by xiiLongOpWorkerManager when they have
/// been named by a xiiLongOpProxy's GetReplicationInfo() function.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiLongOpWorker : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLongOpWorker, xiiReflectedClass);

public:
  /// Called within the engine processes main thread.
  /// The function may lock the xiiWorld from the given scene document and extract vital information.
  /// It should try to be as quick as possible and leave the heavy lifting to Execute(), which will run on a background thread.
  /// If this function return failure, the long op is canceled right away.
  virtual xiiResult InitializeExecution(xiiStreamReader& ref_config, const xiiUuid& documentGuid) { return XII_SUCCESS; }

  /// Executed in a separete thread after InitializeExecution(). This should do the work that takes a while.
  ///
  /// This function may write the result data directly to disk. Everything that is written to \a proxydata
  /// will be transmitted back to the proxy long op and given to xiiLongOpProxy::Finalize(). Since this requires IPC bandwidth
  /// the amount of data should be kept very small (a few KB at most).
  ///
  /// All updates to \a progress will be automatically synchronized back to the editor process and become visible through
  /// the xiiLongOpControllerManager via the xiiLongOpControllerEvent.
  /// Use xiiProgressRange for convenient progress updates.
  virtual xiiResult Execute(xiiProgress& ref_progress, xiiStreamWriter& ref_proxydata) = 0;
};
