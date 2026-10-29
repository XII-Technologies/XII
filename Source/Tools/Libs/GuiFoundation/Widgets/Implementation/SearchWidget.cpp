/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <QKeyEvent>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>

xiiQtSearchWidget::xiiQtSearchWidget(QWidget* pParent) :
  QWidget(pParent)
{
  setLayout(new QHBoxLayout(this));
  setContentsMargins(0, 0, 0, 0);
  layout()->setContentsMargins(0, 0, 0, 0);
  layout()->setSpacing(0);

  {
    m_pLineEdit = new QLineEdit(this);
    m_pLineEdit->setPlaceholderText("Search");
    m_pLineEdit->setAccessibleName(tr("Search"));
    m_pLineEdit->setClearButtonEnabled(true);
    m_pLineEdit->addAction(QIcon(":/GuiFoundation/Icons/Search.svg"), QLineEdit::LeadingPosition);
    m_pLineEdit->installEventFilter(this);

    setFocusProxy(m_pLineEdit);
  }

  layout()->addWidget(m_pLineEdit);

  connect(m_pLineEdit, &QLineEdit::textChanged, this, &xiiQtSearchWidget::onLineEditTextChanged);
}

void xiiQtSearchWidget::setText(const QString& sText)
{
  m_pLineEdit->setText(sText);
}

QString xiiQtSearchWidget::text() const
{
  return m_pLineEdit->text();
}

void xiiQtSearchWidget::setPlaceholderText(const QString& sText)
{
  m_pLineEdit->setPlaceholderText(sText);
  m_pLineEdit->setAccessibleName(sText);
}

void xiiQtSearchWidget::selectAll()
{
  QTimer::singleShot(0, m_pLineEdit, &QLineEdit::selectAll);
}

void xiiQtSearchWidget::onLineEditTextChanged(const QString& text)
{
  Q_EMIT textChanged(text);
}

bool xiiQtSearchWidget::eventFilter(QObject* obj, QEvent* e)
{
  if (obj == m_pLineEdit)
  {
    if (e->type() == QEvent::KeyPress)
    {
      QKeyEvent* pEvent = static_cast<QKeyEvent*>(e);
      if (!m_bCaptureTabKey && (pEvent->key() == Qt::Key_Tab || pEvent->key() == Qt::Key_Backtab))
        return false;

      if (pEvent->key() == Qt::Key_Escape && !text().isEmpty())
      {
        setText("");
        return true;
      }

      if (pEvent->key() == Qt::Key_Return || pEvent->key() == Qt::Key_Enter)
      {
        Q_EMIT enterPressed();
        return true;
      }

      if (pEvent->key() == Qt::Key_Up || pEvent->key() == Qt::Key_Down || pEvent->key() == Qt::Key_Tab || pEvent->key() == Qt::Key_Backtab || pEvent->key() == Qt::Key_F1 || pEvent->key() == Qt::Key_F2 || pEvent->key() == Qt::Key_F3)
      {
        Q_EMIT specialKeyPressed((Qt::Key)pEvent->key());
        return true;
      }
    }
  }

  return false;
}

void xiiQtSearchWidget::showEvent(QShowEvent* e)
{
  QWidget::showEvent(e);

  Q_EMIT visibleEvent();
}
