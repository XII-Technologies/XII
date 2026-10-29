/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QWidget>

class QLineEdit;

/// A search field with an inline clear button and keyboard navigation signals.
///
/// The clear button is visible only when the line edit has text.
/// The text can be cleared by pressing ESC while the line edit has focus.
class XII_GUIFOUNDATION_DLL xiiQtSearchWidget : public QWidget
{
  Q_OBJECT
public:
  explicit xiiQtSearchWidget(QWidget* pParent);

  /// Passed through to the QLineEdit
  void setText(const QString& sText);

  /// Passed through from the QLineEdit
  QString text() const;

  /// Passed through to the QLineEdit
  void setPlaceholderText(const QString& sText);

  /// Passed through to the QLineEdit
  void selectAll();

  /// Popup menus and consoles capture Tab for completion. Panels can opt into normal focus traversal.
  void SetCaptureTabKey(bool bCapture) { m_bCaptureTabKey = bCapture; }

Q_SIGNALS:
  /// Passed through from the QLineEdit
  void textChanged(const QString& sText);

  /// The user pressed the enter key
  void enterPressed();

  /// This signal is sent when certain keys are pressed that could be used by external code to implement additional features.
  /// Currently exposed:
  ///   Qt::Key_Up, Qt::Key_Down
  ///   Qt::Key_Tab, Qt::Key_Backtab (that's SHIFT+Tab)
  ///   Qt::Key_F1, Qt::Key_F2, Qt::Key_F3
  void specialKeyPressed(Qt::Key key);

  void visibleEvent();

private Q_SLOTS:
  void onLineEditTextChanged(const QString& text);

protected:
  virtual bool eventFilter(QObject* obj, QEvent* e) override;
  virtual void showEvent(QShowEvent* e) override;

  QLineEdit* m_pLineEdit;
  bool       m_bCaptureTabKey = true;
};
