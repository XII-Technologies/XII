#pragma once

#include <EditorFramework/ui_SnapSettingsDlg.h>
#include <Foundation/Containers/HybridArray.h>

class QAbstractButton;

class xiiQtSnapSettingsDlg : public QDialog, public Ui_SnapSettingsDlg
{
  Q_OBJECT

public:
  xiiQtSnapSettingsDlg(QWidget* parent);

private Q_SLOTS:
  void on_ButtonBox_clicked(QAbstractButton* button);

private:
  struct KeyValue
  {
    XII_DECLARE_POD_TYPE();

    const char* m_szKey;
    float       m_fValue;
  };

  xiiHybridArray<KeyValue, 16> m_Translation;
  xiiHybridArray<KeyValue, 16> m_Rotation;
  xiiHybridArray<KeyValue, 16> m_Scale;

  void QueryUI();
};
