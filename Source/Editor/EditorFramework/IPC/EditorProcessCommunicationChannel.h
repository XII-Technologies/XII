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
  xiiResult StartClientProcess(const char* szProcess, const QStringList& args, bool bRemote, const xiiRTTI* pFirstAllowedMessageType = nullptr, xiiUInt32 uiMemSize = 1024 * 1024 * 10);

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

  bool IsConnected() const;

  void CloseConnection();

  void TryConnect();

private:
};
