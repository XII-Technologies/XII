/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <EditorPluginScene/ui_ExportAndRunDlg.h>
#include <GameEngine/Configuration/PlayerConfig.h>
#include <QDialog>

class xiiSceneDocument;
class QLineEdit;
class QSpinBox;
class QComboBox;
class QCheckBox;
class QPlainTextEdit;

class xiiQtExportAndRunDlg : public QDialog, public Ui_ExportAndRunDlg
{
  Q_OBJECT

public:
  xiiQtExportAndRunDlg(QWidget* pParent);

  static bool           s_bTransformAll;
  static bool           s_bUpdateThumbnail;
  static bool           s_bCompileCpp;
  bool                  m_bRunAfterExport        = false;
  bool                  m_bShowThumbnailCheckbox = true;
  xiiPlayerLaunchConfig m_Launch;
  xiiString             m_sCurrentScene;
  xiiString             m_sCurrentSourceScene;
  bool                  m_bUseCurrentScene = true;
  xiiString             m_sCmdLine;
  xiiCppSettings        m_CppSettings;

private Q_SLOTS:
  void on_ExportOnly_clicked();
  void on_ExportAndRun_clicked();
  void on_AddToolButton_clicked();
  void on_RemoveToolButton_clicked();
  void on_ToolCombo_currentIndexChanged(int);

private:
  void            PullFromUI();
  void            SelectLaunch();
  void            SaveLaunch();
  xiiPlayerConfig m_ProjectConfig;
  xiiString       m_sConfigPath;
  bool            m_bConfigReadable    = true;
  QComboBox*      m_pLaunchName        = nullptr;
  QLineEdit*      m_pMainScene         = nullptr;
  QLineEdit*      m_pSceneOverride     = nullptr;
  QLineEdit*      m_pProjectOverride   = nullptr;
  QSpinBox*       m_pInstances         = nullptr;
  QComboBox*      m_pRole              = nullptr;
  QLineEdit*      m_pConnect           = nullptr;
  QSpinBox*       m_pBasePort          = nullptr;
  QCheckBox*      m_pArrange           = nullptr;
  QCheckBox*      m_pUseCurrent        = nullptr;
  QPlainTextEdit* m_pArguments         = nullptr;
  QPlainTextEdit* m_pInstanceArguments = nullptr;

protected:
  virtual void showEvent(QShowEvent* e) override;
};
