#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/FileBrowserPropertyWidget.moc.h>
#include <EditorFramework/PropertyGrid/QtFileLineEdit.moc.h>

xiiQtFileLineEdit::xiiQtFileLineEdit(xiiQtFilePropertyWidget* pParent) :
  QLineEdit(pParent)
{
  m_pOwner = pParent;
}

void xiiQtFileLineEdit::dragMoveEvent(QDragMoveEvent* e)
{
  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    if (m_pOwner->IsValidFileReference(qtToXIIString(str)))
      e->acceptProposedAction();

    return;
  }

  QLineEdit::dragMoveEvent(e);
}

void xiiQtFileLineEdit::dragEnterEvent(QDragEnterEvent* e)
{
  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    if (m_pOwner->IsValidFileReference(qtToXIIString(str)))
      e->acceptProposedAction();

    return;
  }

  QLineEdit::dragEnterEvent(e);
}

void xiiQtFileLineEdit::dropEvent(QDropEvent* e)
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
      setText(xiiMakeQString(sPath));
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
