#pragma once

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEngineProcessCommunicationChannel : public xiiProcessCommunicationChannel
{
public:
  xiiResult ConnectToHostProcess();

  bool IsHostAlive() const;

private:
  xiiInt64 m_iHostPID = 0;
};
