/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>

template <typename T>
class QList;
class QString;
using QStringList = QList<QString>;
class QProcess;

class XII_EDITORFRAMEWORK_DLL xiiEditorProcessCommunicationChannel : public xiiProcessCommunicationChannel
{
public:
  ~xiiEditorProcessCommunicationChannel();

  xiiResult StartClientProcess(const char* szProcess, const QStringList& args, xiiProcessMessageDomain domain, const xiiRTTI* pFirstAllowedMessageType = nullptr);

  bool IsClientAlive() const;

  /// Event-driven GUI-thread wait used only during interactive Editor startup. A null type waits
  /// for connection; otherwise dispatches IPC until the requested response or disconnection.
  xiiResult WaitForStartup(const xiiRTTI* pResponseType = nullptr);

  void CloseConnection();

  xiiString GetStdoutContents();

private:
  QProcess* m_pClientProcess = nullptr;
};

class XII_EDITORFRAMEWORK_DLL xiiEditorProcessRemoteCommunicationChannel : public xiiProcessCommunicationChannel
{
public:
  xiiResult ConnectToServer(const char* szAddress);

  void CloseConnection();

  void TryConnect();

private:
  xiiString m_sAddress;
  xiiTime   m_RetryAt;
};
