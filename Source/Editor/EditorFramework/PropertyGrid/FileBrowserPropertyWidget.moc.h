#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

#include <QLineEdit>

class xiiQtFileLineEdit;

class XII_EDITORFRAMEWORK_DLL xiiQtFilePropertyWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtFilePropertyWidget();
  bool IsValidFileReference(xiiStringView sFile) const;

private Q_SLOTS:
  void on_BrowseFile_clicked();

protected slots:
  void on_TextFinished_triggered();
  void on_TextChanged_triggered(const QString& value);
  void OnOpenExplorer();
  void OnCustomAction();
  void OnOpenFile();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

protected:
  QHBoxLayout*       m_pLayout = nullptr;
  xiiQtFileLineEdit* m_pWidget = nullptr;
  QToolButton*       m_pButton = nullptr;
};
