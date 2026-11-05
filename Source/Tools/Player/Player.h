/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/System/Window.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Threading/Mutex.h>
#include <GameEngine/GameApplication/GameApplication.h>

class xiiPlayerApplication : public xiiGameApplication
{
public:
  using SUPER = xiiGameApplication;
  xiiPlayerApplication();
  xiiUniquePtr<xiiWindow> TakeStartupWindow() override;
  Execution               Run() override;

protected:
  void      Run_InputUpdate() override;
  xiiResult BeforeCoreSystemsStartup() override;
  void      AfterCoreSystemsStartup() override;
  void      BeforeHighLevelSystemsShutdown() override;
  void      BeforeCoreSystemsShutdown() override;
  void      AfterCoreSystemsShutdown() override;
  void      Init_FileSystem_ConfigureDataDirs() override;
  void      Init_ConfigureCVars() override;
  void      Init_ConfigureTelemetry() override;

private:
  void DetermineProjectPath();
  void       ReportStatus(xiiStringView sStage, xiiStringView sDetail, float fProgress = -1.0f) override;
  void       LogMessage(const xiiLoggingEventData& e);
  void       UpdateRuntimeSettings();
  xiiWindow* GetPlayerWindow() const;
  xiiResult  ApplyDisplayPreview();
  void       RevertDisplayPreview();
  xiiResult  ResolveLaunchConfiguration();

  xiiUniquePtr<xiiWindow>      m_pStartupWindow;
  xiiString                    m_sWritableDirectory;
  xiiMutex                     m_LogMutex;
  xiiLogWriter::HTML           m_Log;
  xiiEventSubscriptionID       m_LogSubscription = 0;
  xiiOSFile                    m_UserSettingsLock;
  xiiOSFile                    m_TelemetryPortLease;
  xiiString                    m_sOwnedTemporaryDirectory;
  xiiGameApplicationSettings   m_PersistedSettings;
  bool                         m_bRuntimeSettingsCommitted = false;
  bool                         m_bInitializing             = true;
  xiiUInt32                    m_uiInitStep                = 0;
  bool                         m_bShowSettings             = false;
  bool                         m_bDisplayPreview           = false;
  xiiTime                      m_DisplayPreviewDeadline;
  xiiUInt32                    m_uiSelectedSetting = 0;
  xiiGameApplicationSettings   m_PendingSettings;
  xiiWindowCreationDescription m_PendingWindow;
  xiiWindowCreationDescription m_PreviousWindow;
  xiiString                    m_sSettingsMessage;
};
