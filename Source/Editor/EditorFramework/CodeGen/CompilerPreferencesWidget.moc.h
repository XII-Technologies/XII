/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

class XII_EDITORFRAMEWORK_DLL xiiQtCompilerPreferencesWidget : public xiiQtPropertyTypeWidget
{
  Q_OBJECT;

private Q_SLOTS:
  void on_compiler_preset_changed(int index);

public:
  explicit xiiQtCompilerPreferencesWidget();
  virtual ~xiiQtCompilerPreferencesWidget();

  virtual void SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items) override;

protected:
  QComboBox* m_pCompilerPreset;
};
