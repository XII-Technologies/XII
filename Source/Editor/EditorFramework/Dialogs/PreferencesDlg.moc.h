/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/ui_PreferencesDlg.h>
#include <QDialog>

class xiiPreferencesDocument;
class xiiDocumentObject;

/// Detached reflected editing. Apply/OK persist and commit; Cancel discards edits since the last Apply.
class XII_EDITORFRAMEWORK_DLL xiiQtPreferencesDlg : public QDialog, public Ui_xiiQtPreferencesDlg
{
  Q_OBJECT

public:
  explicit xiiQtPreferencesDlg(QWidget* pParent, const xiiDocument* pContext = nullptr);
  ~xiiQtPreferencesDlg();

  xiiResult ApplyAllChanges();

private Q_SLOTS:
  void on_ButtonOk_clicked();
  void on_ButtonApply_clicked();
  void on_ButtonCancel_clicked() { reject(); }
  void on_ButtonReset_clicked();

private:
  xiiUuid NativeToObject(xiiSettings* pSettings);
  void    ReleaseObjects();
  void    FilterNavigation(const QString& text);
  void    SelectionChanged();

  xiiPreferencesDocument*                                      m_pDocument = nullptr;
  xiiMap<xiiUuid, xiiSettings*>                                m_Bindings;
  xiiEvent<const xiiDocument*>::Unsubscriber                   m_InvalidatedSubscription;
  xiiCopyOnBroadcastEvent<const xiiPluginEvent&>::Unsubscriber m_PluginSubscription;
};
