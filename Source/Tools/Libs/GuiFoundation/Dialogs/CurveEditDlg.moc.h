#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <GuiFoundation/ui_CurveEditDlg.h>
#include <QDialog>

class xiiCurveGroupData;
class xiiObjectAccessorBase;
class xiiDocumentObject;

class XII_GUIFOUNDATION_DLL xiiQtCurveEditDlg : public QDialog, Ui_CurveEditDlg
{
  Q_OBJECT
public:
  xiiQtCurveEditDlg(xiiObjectAccessorBase* pObjectAccessor, const xiiDocumentObject* pCurveObject, QWidget* parent);
  ~xiiQtCurveEditDlg();

  static QByteArray GetLastDialogGeometry() { return s_LastDialogGeometry; }

  void SetCurveColor(const xiiColor& color);
  void SetCurveExtents(double fLower, bool bLowerFixed, double fUpper, bool bUpperFixed);
  void SetCurveRanges(double fLower, double fUpper);

  virtual void reject() override;
  virtual void accept() override;

  void cancel();

Q_SIGNALS:

private Q_SLOTS:
  void OnCpMovedEvent(xiiUInt32 curveIdx, xiiUInt32 cpIdx, xiiInt64 iTickX, double newPosY);
  void OnCpDeletedEvent(xiiUInt32 curveIdx, xiiUInt32 cpIdx);
  void OnTangentMovedEvent(xiiUInt32 curveIdx, xiiUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void OnInsertCpEvent(xiiUInt32 uiCurveIdx, xiiInt64 tickX, double value);
  void OnTangentLinkEvent(xiiUInt32 curveIdx, xiiUInt32 cpIdx, bool bLink);
  void OnCpTangentModeEvent(xiiUInt32 curveIdx, xiiUInt32 cpIdx, bool rightTangent, int mode); // xiiCurveTangentMode

  void OnBeginCpChangesEvent(QString name);
  void OnEndCpChangesEvent();

  void OnBeginOperationEvent(QString name);
  void OnEndOperationEvent(bool commit);

  void on_actionUndo_triggered();
  void on_actionRedo_triggered();
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();

private:
  static QByteArray s_LastDialogGeometry;

  void RetrieveCurveState();
  void UpdatePreview();

  double            m_fLowerRange         = -xiiMath::HighValue<double>();
  double            m_fUpperRange         = xiiMath::HighValue<double>();
  double            m_fLowerExtents       = 0.0;
  double            m_fUpperExtents       = 1.0;
  bool              m_bLowerFixed         = false;
  bool              m_bUpperFixed         = false;
  bool              m_bCurveLengthIsFixed = false;
  xiiCurveGroupData m_Curves;
  xiiUInt32         m_uiActionsUndoBaseline = 0;

  QShortcut* m_pShortcutUndo = nullptr;
  QShortcut* m_pShortcutRedo = nullptr;

  xiiObjectAccessorBase*   m_pObjectAccessor = nullptr;
  const xiiDocumentObject* m_pCurveObject    = nullptr;

protected:
  virtual void closeEvent(QCloseEvent* e) override;
  virtual void showEvent(QShowEvent* e) override;
};
