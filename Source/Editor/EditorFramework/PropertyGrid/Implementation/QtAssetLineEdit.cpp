#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>

xiiQtAssetLineEdit::xiiQtAssetLineEdit(QWidget* parent /*= nullptr*/) :
  QLineEdit(parent)
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
