/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/ui_LogPanel.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class xiiQtLogModel;
struct xiiLoggingEventData;
class xiiPreferences;

/// The application wide panel that shows the engine log output and the editor log output
class XII_EDITORFRAMEWORK_DLL xiiQtLogPanel : public xiiQtApplicationPanel, public Ui_LogPanel
{
  Q_OBJECT

  XII_DECLARE_SINGLETON(xiiQtLogPanel);

public:
  xiiQtLogPanel(ads::CDockManager* pDockManager);
  ~xiiQtLogPanel();

protected:
  virtual void ToolsProjectEventHandler(const xiiToolsProjectEvent& e) override;

private Q_SLOTS:
  void OnNewWarningsOrErrors(xiiStringView sText, bool bError);

private:
  void LogWriter(const xiiLoggingEventData& e);
  void EngineProcessMsgHandler(const xiiEditorEngineProcessConnection::Event& e);
  void UiServiceEventHandler(const xiiQtUiServices::Event& e);
  void OnPreferenceChange(xiiPreferences* pref);

  xiiUInt32 m_uiIgnoredNumErrors  = 0;
  xiiUInt32 m_uiIgnoreNumWarnings = 0;
  xiiUInt32 m_uiKnownNumErrors    = 0;
  xiiUInt32 m_uiKnownNumWarnings  = 0;

  bool m_bCombineLogs = true;
};
