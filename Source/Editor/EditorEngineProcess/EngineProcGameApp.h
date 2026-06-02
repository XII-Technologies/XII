/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <EditorEngineProcessFramework/LongOps/LongOpWorkerManager.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameApplication/GameApplication.h>

class xiiEditorEngineProcessApp;
class xiiDocumentOpenMsgToEngine;
class xiiEngineProcessDocumentContext;
class xiiResourceUpdateMsgToEngine;
class xiiRestoreResourceMsgToEngine;

class xiiEngineProcessGameApplication : public xiiGameApplication
{
public:
  using SUPER = xiiGameApplication;

  xiiEngineProcessGameApplication();
  ~xiiEngineProcessGameApplication();

  virtual xiiResult BeforeCoreSystemsStartup() override;
  virtual void      AfterCoreSystemsStartup() override;

  virtual void BeforeCoreSystemsShutdown() override;

  virtual xiiApplication::Execution Run() override;

  void LogWriter(const xiiLoggingEventData& e);

  virtual bool ShouldApplicaitonQuit() const override;

protected:
  virtual void                                    BaseInit_ConfigureLogging() override;
  virtual void                                    Deinit_ShutdownLogging() override;
  virtual void                                    Init_FileSystem_ConfigureDataDirs() override;
  virtual bool                                    Run_ProcessApplicationInput() override;
  virtual xiiUniquePtr<xiiEditorEngineProcessApp> CreateEngineProcessApp();

  virtual void ActivateGameStateAtStartup() override
  {
    // Do nothing.
  }

private:
  void        ConnectToHost();
  void        DisableErrorReport();
  void        WaitForDebugger();
  static bool EditorAssertHandler(const char* szSourceFile, xiiUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg);
  void        AddEditorAssertHandler();
  void        RemoveEditorAssertHandler();

  bool ProcessIPCMessages(bool bPendingOpInProgress);
  void SendProjectReadyMessage();
  void SendReflectionInformation();
  void EventHandlerIPC(const xiiEngineProcessCommunicationChannel::Event& e);
  void EventHandlerCVar(const xiiCVarEvent& e);
  void EventHandlerCVarPlugin(const xiiPluginEvent& e);
  void TransmitCVar(const xiiCVar* pCVar);

  void HandleResourceUpdateMsg(const xiiResourceUpdateMsgToEngine& msg);
  void HandleResourceRestoreMsg(const xiiRestoreResourceMsgToEngine& msg);

  xiiEngineProcessDocumentContext* CreateDocumentContext(const xiiDocumentOpenMsgToEngine* pMsg);

  virtual void Init_LoadProjectPlugins() override;

  virtual xiiString FindProjectDirectory() const override;

  xiiString                               m_sProjectDirectory;
  xiiApplicationFileSystemConfig          m_CustomFileSystemConfig;
  xiiApplicationPluginConfig              m_CustomPluginConfig;
  xiiEngineProcessCommunicationChannel    m_IPC;
  xiiUniquePtr<xiiEditorEngineProcessApp> m_pApp;
  xiiLongOpWorkerManager                  m_LongOpWorkerManager;
  xiiLogWriter::HTML                      m_LogHTML;

  xiiUInt32 m_uiRedrawCountReceived = 0;
  xiiUInt32 m_uiRedrawCountExecuted = 0;
};
