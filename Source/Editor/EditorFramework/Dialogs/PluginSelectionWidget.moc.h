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
};
