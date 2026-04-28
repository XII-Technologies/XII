/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_PreferencesDlg.h>
#include <Foundation/Strings/String.h>

#include <QDialog>

class xiiPreferencesDocument;
class xiiPreferences;
class xiiQtDocumentTreeView;

class XII_EDITORFRAMEWORK_DLL xiiQtPreferencesDlg : public QDialog, public Ui_xiiQtPreferencesDlg
{
public:
  Q_OBJECT

public:
  xiiQtPreferencesDlg(QWidget* pParent);
  ~xiiQtPreferencesDlg();

  xiiUuid NativeToObject(xiiPreferences* pPreferences);
  void    ObjectToNative(xiiUuid objectGuid, const xiiDocument* pPrefDocument);

private Q_SLOTS:
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked() { reject(); }

private:
  void RegisterAllPreferenceTypes();
  void AllPreferencesToObject();
  void PropertyChangedEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void ApplyAllChanges();

  xiiPreferencesDocument*             m_pDocument;
  xiiMap<xiiUuid, const xiiDocument*> m_DocumentBinding;
};
