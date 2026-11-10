/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEngineProcessCommunicationChannel : public xiiProcessCommunicationChannel
{
public:
  xiiResult ConnectToHostProcess();

  /// Releases the protocol and channel while the Core message loop is still running.
  /// Safe to call repeatedly, including before connecting.
  void CloseConnection();

  bool IsHostAlive() const;

private:
  xiiInt64 m_iHostPID = 0;
};
