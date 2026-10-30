/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/Settings/EditorSettings.h>
#include <EditorFramework/ui_ConfigurationDlg.h>
#include <Foundation/Configuration/Plugin.h>
#include <QDialog>
#include <QPointer>
#include <QSet>
#include <ToolsFoundation/Project/ToolsProject.h>

class xiiEditorSettingsDocument;
class xiiDocumentObject;

/// Detached reflected editing. Apply/OK persist and commit; Cancel discards edits since the last Apply.
class XII_EDITORFRAMEWORK_DLL xiiQtConfigurationDlg : public QDialog, public Ui_xiiQtConfigurationDlg
{
  Q_OBJECT

public:
  enum class Section
  {
    ProjectSettings,
    EditorPreferences,
    Plugins
  };
  static void Show(Section section, const xiiDocument* pContext = nullptr, xiiStringView sPage = {});
  void        SelectSection(Section section, xiiStringView sPage = {});

  explicit xiiQtConfigurationDlg(QWidget* pParent, const xiiDocument* pContext = nullptr, Section section = Section::EditorPreferences);
  ~xiiQtConfigurationDlg();

  xiiResult ApplyAllChanges();

private Q_SLOTS:
  void on_ButtonOk_clicked();
  void on_ButtonApply_clicked();
  void on_ButtonCancel_clicked() { reject(); }
  void on_ButtonReset_clicked();

private:
  xiiUuid   NativeToObject(xiiSettings* pSettings);
  void      ReleaseObjects();
  void      FilterNavigation(const QString& text);
  void      SelectionChanged();
  QWidget*  CreatePage(int iPage);
  void      TrackPageChanges(QWidget* pPage);
  xiiResult ApplyPages();

  const xiiDocument*                                            m_pContext = nullptr;
  xiiMap<int, QWidget*>                                         m_Pages;
  QSet<QWidget*>                                                m_ModifiedPages;
  xiiPluginBundleSet                                            m_Plugins;
  xiiEvent<const xiiToolsProjectEvent&, xiiMutex>::Unsubscriber m_ProjectSubscription;

  xiiEditorSettingsDocument*                                   m_pDocument = nullptr;
  xiiMap<xiiUuid, xiiSettings*>                                m_Bindings;
  xiiEvent<const xiiDocument*>::Unsubscriber                   m_InvalidatedSubscription;
  xiiCopyOnBroadcastEvent<const xiiPluginEvent&>::Unsubscriber m_PluginSubscription;
};
