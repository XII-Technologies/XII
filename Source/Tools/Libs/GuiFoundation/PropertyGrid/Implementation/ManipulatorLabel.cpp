/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/Implementation/ManipulatorLabel.moc.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QFont>
#include <qevent.h>

xiiQtManipulatorLabel::xiiQtManipulatorLabel(QWidget* pParent, Qt::WindowFlags f) :
  QLabel(pParent, f), m_pItems(nullptr), m_pManipulator(nullptr), m_bActive(false)
{
  setCursor(Qt::WhatsThisCursor);
}

xiiQtManipulatorLabel::xiiQtManipulatorLabel(const QString& sText, QWidget* pParent, Qt::WindowFlags f) :
  QLabel(sText, pParent, f), m_pItems(nullptr), m_pManipulator(nullptr), m_bActive(false), m_bIsDefault(true)
{
}

const xiiManipulatorAttribute* xiiQtManipulatorLabel::GetManipulator() const
{
  return m_pManipulator;
}

void xiiQtManipulatorLabel::SetManipulator(const xiiManipulatorAttribute* pManipulator)
{
  m_pManipulator = pManipulator;

  if (m_pManipulator)
  {
    setCursor(Qt::PointingHandCursor);
    setForegroundRole(QPalette::ColorRole::Link);
  }
}

bool xiiQtManipulatorLabel::GetManipulatorActive() const
{
  return m_bActive;
}

void xiiQtManipulatorLabel::SetManipulatorActive(bool bActive)
{
  m_bActive = bActive;

  if (m_pManipulator)
  {
    setForegroundRole(m_bActive ? QPalette::ColorRole::LinkVisited : QPalette::ColorRole::Link);
  }
}

void xiiQtManipulatorLabel::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  m_pItems = &items;
}


void xiiQtManipulatorLabel::SetIsDefault(bool bIsDefault)
{
  if (m_bIsDefault != bIsDefault)
  {
    m_bIsDefault = bIsDefault;
    m_Font.setBold(!m_bIsDefault);
    setFont(m_Font);
  }
}


void xiiQtManipulatorLabel::contextMenuEvent(QContextMenuEvent* ev)
{
  Q_EMIT customContextMenuRequested(ev->globalPos());
}

void xiiQtManipulatorLabel::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set font.
  setFont(m_Font);
  QLabel::showEvent(event);
}

void xiiQtManipulatorLabel::mousePressEvent(QMouseEvent* ev)
{
  if (ev->button() != Qt::LeftButton)
    return;

  if (m_pManipulator == nullptr)
    return;

  const xiiDocument* pDoc = (*m_pItems)[0].m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  if (m_bActive)
    xiiManipulatorManager::GetSingleton()->ClearActiveManipulator(pDoc);
  else
    xiiManipulatorManager::GetSingleton()->SetActiveManipulator(pDoc, m_pManipulator, *m_pItems);
}

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
void xiiQtManipulatorLabel::enterEvent(QEnterEvent* ev)
#else
void xiiQtManipulatorLabel::enterEvent(QEvent* ev)
#endif
{
  if (m_pManipulator)
  {
    m_Font.setUnderline(true);
    setFont(m_Font);
  }

  QLabel::enterEvent(ev);
}

void xiiQtManipulatorLabel::leaveEvent(QEvent* ev)
{
  if (m_pManipulator)
  {
    m_Font.setUnderline(false);
    setFont(m_Font);
  }

  QLabel::leaveEvent(ev);
}
