#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class xiiQtCVarWidget;

class XII_EDITORFRAMEWORK_DLL xiiQtCVarPanel : public xiiQtApplicationPanel
{
  Q_OBJECT

  XII_DECLARE_SINGLETON(xiiQtCVarPanel);

public:
  xiiQtCVarPanel();
  ~xiiQtCVarPanel();

protected:
  virtual void ToolsProjectEventHandler(const xiiToolsProjectEvent& e) override;

private Q_SLOTS:
  void UpdateUI();
  void BoolChanged(const char* szCVar, bool newValue);
  void FloatChanged(const char* szCVar, float newValue);
  void IntChanged(const char* szCVar, int newValue);
  void StringChanged(const char* szCVar, const char* newValue);

private:
  void EngineProcessMsgHandler(const xiiEditorEngineProcessConnection::Event& e);

  xiiQtCVarWidget* m_pCVarWidget = nullptr;

  xiiMap<xiiString, xiiCVarWidgetData> m_EngineCVarState;

  bool             m_bUpdateUI      = false;
  bool             m_bRebuildUI     = false;
  bool             m_bUpdateConsole = false;
  xiiStringBuilder m_sCommandResult;
};
