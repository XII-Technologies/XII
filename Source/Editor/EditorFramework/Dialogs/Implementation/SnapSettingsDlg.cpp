#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/SnapSettingsDlg.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>

xiiQtSnapSettingsDlg::xiiQtSnapSettingsDlg(QWidget* pParent) :
  QDialog(pParent)
{
  setupUi(this);

  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0", 0.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_01", 0.01f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_05", 0.05f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_1", 0.1f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_125", 0.125f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_2", 0.2f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_25", 0.25f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_5", 0.5f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.1", 1.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.2", 2.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.4", 4.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.5", 5.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.8", 8.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.10", 10.0f});

  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.0_Degree", 0.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.1_Degree", 1.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.5_Degree", 5.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.10_Degree", 10.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.15_Degree", 15.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.22_5_Degree", 22.5f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.30_Degree", 30.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.45_Degree", 45.0f});

  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.0", 0.0f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.0_125", 0.125f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.0_25", 0.25f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.0_5", 0.5f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.1", 1.0f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.2", 2.0f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.4", 4.0f});

  xiiUInt32 uiSelectedT = 0;
  xiiUInt32 uiSelectedR = 0;
  xiiUInt32 uiSelectedS = 0;

  for (xiiUInt32 i = 0; i < m_Translation.GetCount(); ++i)
  {
    TranslationSnap->addItem(xiiMakeQString(xiiTranslate(m_Translation[i].m_szKey)));

    if (xiiSnapProvider::GetTranslationSnapValue() == m_Translation[i].m_fValue)
      uiSelectedT = i;
  }

  for (xiiUInt32 i = 0; i < m_Rotation.GetCount(); ++i)
  {
    RotationSnap->addItem(xiiMakeQString(xiiTranslate(m_Rotation[i].m_szKey)));

    if (xiiSnapProvider::GetRotationSnapValue() == xiiAngle::MakeFromDegree(m_Rotation[i].m_fValue))
      uiSelectedR = i;
  }

  for (xiiUInt32 i = 0; i < m_Scale.GetCount(); ++i)
  {
    ScaleSnap->addItem(xiiMakeQString(xiiTranslate(m_Scale[i].m_szKey)));

    if (xiiSnapProvider::GetScaleSnapValue() == m_Scale[i].m_fValue)
      uiSelectedS = i;
  }

  TranslationSnap->setCurrentIndex(uiSelectedT);
  RotationSnap->setCurrentIndex(uiSelectedR);
  ScaleSnap->setCurrentIndex(uiSelectedS);
}

void xiiQtSnapSettingsDlg::QueryUI()
{
  xiiSnapProvider::SetTranslationSnapValue(m_Translation[TranslationSnap->currentIndex()].m_fValue);
  xiiSnapProvider::SetRotationSnapValue(xiiAngle::MakeFromDegree(m_Rotation[RotationSnap->currentIndex()].m_fValue));
  xiiSnapProvider::SetScaleSnapValue(m_Scale[ScaleSnap->currentIndex()].m_fValue);
}

void xiiQtSnapSettingsDlg::on_ButtonBox_clicked(QAbstractButton* button)
{
  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Ok))
  {
    QueryUI();
    accept();
    return;
  }

  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Cancel))
  {
    reject();
    return;
  }
}
