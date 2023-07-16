#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QLabel>

class xiiManipulatorAttribute;

class xiiQtManipulatorLabel : public QLabel
{
  Q_OBJECT
public:
  explicit xiiQtManipulatorLabel(QWidget* pParent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  explicit xiiQtManipulatorLabel(const QString& sText, QWidget* pParent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

  const xiiManipulatorAttribute* GetManipulator() const;
  void                           SetManipulator(const xiiManipulatorAttribute* pManipulator);

  bool GetManipulatorActive() const;
  void SetManipulatorActive(bool bActive);

  void SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items);

  void SetIsDefault(bool bIsDefault);

protected:
  virtual void contextMenuEvent(QContextMenuEvent* ev) override;
  virtual void showEvent(QShowEvent* event) override;

private:
  virtual void mousePressEvent(QMouseEvent* ev) override;

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
  virtual void enterEvent(QEnterEvent* ev) override;
#else
  virtual void enterEvent(QEvent* ev) override;
#endif

  virtual void leaveEvent(QEvent* ev) override;

private:
  const xiiHybridArray<xiiPropertySelection, 8>* m_pItems;
  const xiiManipulatorAttribute*                 m_pManipulator;
  QFont                                          m_Font;
  bool                                           m_bActive;
  bool                                           m_bIsDefault;
};
