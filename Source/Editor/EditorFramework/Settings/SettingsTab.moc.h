#pragma once

#include <EditorFramework/ui_SettingsTab.h>
#include <Foundation/Configuration/Plugin.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class xiiQtSettingsTab : public xiiQtDocumentWindow, Ui_SettingsTab
{
  Q_OBJECT

  XII_DECLARE_SINGLETON(xiiQtSettingsTab);

public:
  xiiQtSettingsTab();
  ~xiiQtSettingsTab();

  virtual xiiString GetWindowIcon() const override;
  virtual xiiString GetDisplayNameShort() const override;

  virtual xiiStringView GetWindowLayoutGroupName() const override { return "Settings"; }

protected Q_SLOTS:
  void on_OpenScene_clicked();
  void on_OpenProject_clicked();
  void on_GettingStarted_clicked();

private:
  virtual bool InternalCanCloseWindow() override;
  virtual void InternalCloseDocumentWindow() override;

  void ToolsProjectEventHandler(const xiiToolsProjectEvent& e);
};
