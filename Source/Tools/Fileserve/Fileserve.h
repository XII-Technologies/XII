/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Types/UniquePtr.h>

/// A stand-alone application for the xiiFileServer.
///
/// If XII_USE_QT is defined, the GUI from the EditorPluginFileserve is used. Otherwise the server runs as a console application.
///
/// If the command line option "-fs_wait_timeout seconds" is specified, the server will wait for a limited time for any client to
/// connect and close automatically, if no connection is established. Once a client connects, the timeout becomes irrelevant.
/// If the command line option "-fs_close_timeout seconds" is specified, the application will automatically shut down when no
/// client is connected anymore and a certain timeout is reached. Once a client connects, the timeout is reset.
/// This timeout has no effect as long as no client has connected.
class xiiFileserverApp : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  xiiFileserverApp() :
    xiiApplication("Fileserve")
  {
  }

  virtual xiiResult BeforeCoreSystemsStartup() override;
  virtual void      AfterCoreSystemsStartup() override;
  virtual void      BeforeCoreSystemsShutdown() override;

  virtual xiiApplication::Execution Run() override;
  void                              FileserverEventHandlerConsole(const xiiFileserverEvent& e);
  void                              FileserverEventHandler(const xiiFileserverEvent& e);

  void ShaderMessageHandler(xiiFileserveClientContext& ref_ctxt, xiiRemoteMessage& ref_msg, xiiRemoteInterface& ref_clientChannel, xiiDelegate<void(const char*)> logActivity);

  void SetStyleSheet();

  xiiUInt32 m_uiSleepCounter = 0;
  xiiUInt32 m_uiConnections  = 0;
  xiiTime   m_CloseAppTimeout;
  xiiTime   m_TimeTillClosing;
};
