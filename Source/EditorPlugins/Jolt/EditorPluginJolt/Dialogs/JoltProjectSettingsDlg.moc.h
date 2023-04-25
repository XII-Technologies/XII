#pragma once

#include <EditorPluginJolt/EditorPluginJoltDLL.h>
#include <EditorPluginJolt/ui_JoltProjectSettingsDlg.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <QDialog>

class xiiQtJoltProjectSettingsDlg : public QDialog, public Ui_JoltProjectSettingsDlg
{
public:
  Q_OBJECT

public:
  xiiQtJoltProjectSettingsDlg(QWidget* pParent);

  static void EnsureConfigFileExists();

private Q_SLOTS:
  void onCheckBoxClicked(bool checked);
  void on_DefaultButtons_clicked(QAbstractButton* pButton);
  void on_ButtonAddLayer_clicked();
  void on_ButtonRemoveLayer_clicked();
  void on_ButtonRenameLayer_clicked();
  void on_FilterTable_itemSelectionChanged();

private:
  void      SetupTable();
  xiiResult Save();
  xiiResult Load();

  xiiUInt32                m_IndexRemap[32];
  xiiCollisionFilterConfig m_Config;
  xiiCollisionFilterConfig m_ConfigReset;
};
