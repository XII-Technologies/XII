/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>

xiiQtAssetLineEdit::xiiQtAssetLineEdit(QWidget* pParent /*= nullptr*/) :
  QLineEdit(pParent)
{
}

void xiiQtAssetLineEdit::dragMoveEvent(QDragMoveEvent* e)
{
  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    if (m_pOwner->IsValidAssetType(str.toUtf8().data()))
      e->acceptProposedAction();

    return;
  }

  QLineEdit::dragMoveEvent(e);
}

void xiiQtAssetLineEdit::dragEnterEvent(QDragEnterEvent* e)
{
  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    if (m_pOwner->IsValidAssetType(str.toUtf8().data()))
      e->acceptProposedAction();

    return;
  }

  QLineEdit::dragEnterEvent(e);
}

void xiiQtAssetLineEdit::dropEvent(QDropEvent* e)
{
  if (e->source() == this)
  {
    QLineEdit::dropEvent(e);
    return;
  }

  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    xiiString sPath = str.toUtf8().data();
    if (xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sPath))
    {
      setText(QString::fromUtf8(sPath.GetData()));
    }
    else
      setText(QString());

    return;
  }


  if (e->mimeData()->hasText())
  {
    QString str = e->mimeData()->text();

    xiiString sPath = str.toUtf8().data();
    if (xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sPath))
    {
      setText(QString::fromUtf8(sPath.GetData()));
    }
    else
      setText(QString());

    return;
  }
}

void xiiQtAssetLineEdit::paintEvent(QPaintEvent* e)
{
  if (hasFocus())
  {
    QLineEdit::paintEvent(e);
  }
  else
  {
    QPainter p(this);

    // Paint background
    QStyleOptionFrame panel;
    initStyleOption(&panel);
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &p, this);

    // Clip to line edit contents
    QRect r       = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
    auto  margins = textMargins();
    r             = r.marginsRemoved(margins);
    p.setClipRect(r);

    // Render asset name
    xiiStringBuilder sText = qtToXIIString(text());
    if (sText.IsEmpty())
    {
      sText = qtToXIIString(placeholderText());
    }

    xiiStringView sFinalText = sText;

    if (m_pOwner->IsValidAssetType(sText))
    {
      if (const char* szPipe = sFinalText.FindLastSubString("|"))
      {
        sFinalText = xiiStringView(szPipe + 1);
      }
      else
      {
        sFinalText = sFinalText.GetFileName();
      }
    }

    r.adjust(2, 0, 2, 0);
    QTextOption opt(Qt::AlignLeft | Qt::AlignVCenter);
    opt.setWrapMode(QTextOption::NoWrap);
    p.drawText(r, xiiMakeQString(sFinalText), opt);
  }
}

void xiiQtAssetLineEdit::mousePressEvent(QMouseEvent* e)
{
  QLineEdit::mousePressEvent(e);

  if ((e->button() == Qt::MouseButton::LeftButton && e->modifiers().testFlag(Qt::ControlModifier)) || (e->button() == Qt::MouseButton::MiddleButton))
  {
    Q_EMIT OpenAsset();
    return;
  }

  if ((e->button() == Qt::MouseButton::LeftButton && e->modifiers().testFlag(Qt::ShiftModifier)))
  {
    Q_EMIT SelectAsset();
    return;
  }
}
