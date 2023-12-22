#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/IPC/EngineProcessConnection.h>
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
  void BoolChanged(xiiStringView sCVar, bool newValue);
  void FloatChanged(xiiStringView sCVar, float newValue);
  void IntChanged(xiiStringView sCVar, int newValue);
  void StringChanged(xiiStringView sCVar, xiiStringView sNewValue);

private:
  void EngineProcessMsgHandler(const xiiEditorEngineProcessConnection::Event& e);

  xiiQtCVarWidget* m_pCVarWidget = nullptr;

  xiiMap<xiiString, xiiCVarWidgetData> m_EngineCVarState;

  bool             m_bUpdateUI      = false;
  bool             m_bRebuildUI     = false;
  bool             m_bUpdateConsole = false;
  xiiStringBuilder m_sCommandResult;
};
