/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_PluginSelectionWidget.h>
#include <Foundation/Strings/String.h>

#include <QWidget>

struct xiiPluginBundleSet;
struct xiiPluginBundle;

class XII_EDITORFRAMEWORK_DLL xiiQtPluginSelectionWidget : public QWidget, public Ui_PluginSelectionWidget
{
public:
  Q_OBJECT

public:
  xiiQtPluginSelectionWidget(QWidget* pParent);
  ~xiiQtPluginSelectionWidget();

  void SetPluginSet(xiiPluginBundleSet* pPluginSet);
  void SyncStateToSet();
  void SelectTemplate(const char* szTemplate);
  void      Filter(const QString& text);
  void      SetReadOnly(bool bReadOnly);
  xiiResult ValidateSelection(xiiStringBuilder& out_reason) const;

private Q_SLOTS:
  void on_PluginsList_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
  void on_PluginsList_itemChanged(QListWidgetItem* item);
  void on_Template_currentIndexChanged(int index);

private:
  struct State
  {
    xiiString        m_sID;
    xiiPluginBundle* m_pInfo         = nullptr;
    bool             m_bLoadCopy     = false;
    bool             m_bSelected     = false;
    bool             m_bIsDependency = false;
  };

  void UpdateInternalState();
  void ApplyRequired(xiiArrayPtr<xiiString> required);

  xiiHybridArray<State, 8> m_States;
  xiiPluginBundleSet*      m_pPluginSet = nullptr;
  QComboBox*               m_pCategory  = nullptr;
  QString                  m_Filter;
  bool                     m_bReadOnly = false;
};
