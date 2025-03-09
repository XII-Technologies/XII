#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

class XII_EDITORFRAMEWORK_DLL xiiQtCodeEditorPreferencesWidget : public xiiQtPropertyTypeWidget
{
  Q_OBJECT;

private Q_SLOTS:
  void on_code_editor_changed(int index);

public:
  explicit xiiQtCodeEditorPreferencesWidget();
  virtual ~xiiQtCodeEditorPreferencesWidget();

  virtual void SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items) override;

protected:
  QComboBox* m_pCodeEditor;
};
