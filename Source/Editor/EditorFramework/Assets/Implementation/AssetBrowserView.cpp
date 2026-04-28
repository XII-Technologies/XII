/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetBrowserView.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

xiiQtAssetBrowserView::xiiQtAssetBrowserView(QWidget* pParent) :
  xiiQtItemView<QListView>(pParent)
{
  m_iIconSizePercentage = 100;
  m_pDelegate           = new xiiQtIconViewDelegate(this);

  SetDialogMode(false);

  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
  setViewMode(QListView::ViewMode::IconMode);
  setUniformItemSizes(true);
  setResizeMode(QListView::ResizeMode::Adjust);

  setItemDelegate(m_pDelegate);
  SetIconScale(m_iIconSizePercentage);
}

void xiiQtAssetBrowserView::startDrag(Qt::DropActions supportedActions)
{
  // overridden so that we can get rid of the preview image

  QModelIndexList indexes = selectedIndexes();
  if (indexes.count() > 0)
  {
    QMimeData* data = model()->mimeData(indexes);
    if (!data)
    {
      return;
    }

    QDrag* drag = new QDrag(this);
    drag->setMimeData(data);

    drag->exec(supportedActions, Qt::MoveAction);
  }
}

void xiiQtAssetBrowserView::SetDialogMode(bool bDialogMode)
{
  m_bDialogMode = bDialogMode;

  if (m_bDialogMode)
  {
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pDelegate->SetDrawTransformState(false);
    setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
    setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  }
  else
  {
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    m_pDelegate->SetDrawTransformState(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  }
}

void xiiQtAssetBrowserView::SetIconMode(bool bIconMode)
{
  if (bIconMode)
  {
    setViewMode(QListView::ViewMode::IconMode);
    SetIconScale(m_iIconSizePercentage);
  }
  else
  {
    setViewMode(QListView::ViewMode::ListMode);
    setGridSize(QSize());
  }
}

void xiiQtAssetBrowserView::SetIconScale(xiiInt32 iIconSizePercentage)
{
  m_iIconSizePercentage = xiiMath::Clamp(iIconSizePercentage, 10, 100);
  m_pDelegate->SetIconScale(m_iIconSizePercentage);

  if (viewMode() != QListView::ViewMode::IconMode)
    return;

  setGridSize(m_pDelegate->sizeHint(QStyleOptionViewItem(), QModelIndex()));
}

xiiInt32 xiiQtAssetBrowserView::GetIconScale() const
{
  return m_iIconSizePercentage;
}

void xiiQtAssetBrowserView::dragEnterEvent(QDragEnterEvent* pEvent)
{
  if (pEvent->source())
    pEvent->acceptProposedAction();
}

void xiiQtAssetBrowserView::dragMoveEvent(QDragMoveEvent* pEvent)
{
  pEvent->acceptProposedAction();
}

void xiiQtAssetBrowserView::dragLeaveEvent(QDragLeaveEvent* pEvent)
{
  pEvent->accept();
}

static void NotifyFileChanges(xiiArrayPtr<xiiString> files)
{
  for (const auto& file : files)
  {
    xiiFileSystemModel::GetSingleton()->NotifyOfChange(file);
  }
}

void xiiQtAssetBrowserView::dropEvent(QDropEvent* pEvent)
{
  if (!pEvent->mimeData()->hasUrls())
    return;

  QList<QUrl>     paths           = pEvent->mimeData()->urls();
  const xiiString targetDirectory = indexAt(pEvent->position().toPoint()).data(xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  if (targetDirectory.IsEmpty())
  {
    return;
  }

  xiiHybridArray<xiiString, 32> touchedFiles;
  // make sure to notify the filesystem of files and folders that were touched
  XII_SCOPE_EXIT(NotifyFileChanges(touchedFiles));

  for (auto it = paths.begin(); it != paths.end(); it++)
  {
    xiiStringBuilder src = it->path().toUtf8().constData();
    src.TrimWordStart("/"); // remove '/' at start
    src.MakeCleanPath();

    xiiStringBuilder dst = targetDirectory;
    dst.MakeCleanPath();

    // prevent moving stuff into itself
    if (src == dst)
      continue;

    // don't allow dropping anything onto an existing file
    if (xiiOSFile::ExistsFile(dst))
      continue;

    dst.AppendPath(qtToXIIString(it->fileName()));

    if (xiiOSFile::ExistsDirectory(src))
    {
      if (xiiOSFile::ExistsDirectory(dst)) // ask to overwrite if target already exists
      {
        const int res = xiiQtUiServices::MessageBoxQuestion(xiiFmt("Directory already exists:\n\n'{}'\n\nOverwrite files inside directory?", dst), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);

        if (res == QMessageBox::Cancel)
          return;

        if (res == QMessageBox::No)
          continue;
      }

      if (xiiOSFile::CopyFolder(src, dst, &touchedFiles).Failed())
      {
        xiiQtUiServices::MessageBoxWarning(xiiFmt("Failed to copy folder:\n\n'{}'\n\nto\n\n'{}'\n\nAborting operation.", src, dst));
        return;
      }

      touchedFiles.PushBack(dst);

      if (xiiOSFile::DeleteFolder(src).Failed())
      {
        xiiQtUiServices::MessageBoxWarning(xiiFmt("Failed to remove folder:\n\n'{}'\n\nAborting operation.", src));
        return;
      }
    }
    else if (xiiOSFile::ExistsFile(src))
    {
      if (xiiOSFile::ExistsFile(dst)) // ask to overwrite if target already exists
      {
        const int res = xiiQtUiServices::MessageBoxQuestion(xiiFmt("The file already exists:\n\n'{}'\n\nOverwrite file?", dst), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);

        if (res == QMessageBox::Cancel)
          return;

        if (res == QMessageBox::No)
          continue;

        xiiOSFile::DeleteFile(dst).IgnoreResult();
      }

      touchedFiles.PushBack(src);
      touchedFiles.PushBack(dst);

      if (xiiOSFile::MoveFileOrDirectory(src, dst).Failed())
      {
        xiiQtUiServices::MessageBoxWarning(xiiFmt("Failed to move file:\n\n'{}'\n\nto\n\n'{}'\n\nAborting operation.", src, dst));
        return;
      }
    }
  }
}

void xiiQtAssetBrowserView::wheelEvent(QWheelEvent* pEvent)
{
  if (pEvent->modifiers() == Qt::CTRL)
  {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    xiiInt32 iDelta = pEvent->angleDelta().y() > 0 ? 5 : -5;
#else
    xiiInt32 iDelta = pEvent->delta() > 0 ? 5 : -5;
#endif
    SetIconScale(m_iIconSizePercentage + iDelta);
    Q_EMIT ViewZoomed(m_iIconSizePercentage);
    return;
  }

  QListView::wheelEvent(pEvent);
}

void xiiQtAssetBrowserView::mouseDoubleClickEvent(QMouseEvent* pEvent)
{
  if (pEvent->button() == Qt::MouseButton::BackButton)
  {
    pEvent->ignore();
    return;
  }

  QListView::mouseDoubleClickEvent(pEvent);
}

void xiiQtAssetBrowserView::mousePressEvent(QMouseEvent* pEvent)
{
  if (pEvent->button() == Qt::MouseButton::BackButton)
  {
    pEvent->ignore();
    return;
  }

  QListView::mousePressEvent(pEvent);
}

void xiiQtAssetBrowserView::mouseMoveEvent(QMouseEvent* pEvent)
{
  // Only allow dragging with left mouse button.
  if (state() == DraggingState && !pEvent->buttons().testFlag(Qt::MouseButton::LeftButton))
    return;

  QListView::mouseMoveEvent(pEvent);
}

xiiQtIconViewDelegate::xiiQtIconViewDelegate(xiiQtAssetBrowserView* pParent) :
  xiiQtItemDelegate(pParent)
{
  m_bDrawTransformState = true;
  m_iIconSizePercentage = 100;
  m_pView               = pParent;
}

void xiiQtIconViewDelegate::SetIconScale(xiiInt32 iIconSizePercentage)
{
  m_iIconSizePercentage = iIconSizePercentage;
}

bool xiiQtIconViewDelegate::mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& opt, const QModelIndex& index)
{
  const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)index.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
  if (!itemType.IsSet(xiiAssetBrowserItemFlags::Asset))
    return false;

  const xiiUInt32 uiThumbnailSize = ThumbnailSize();
  QRect           thumbnailRect   = opt.rect.adjusted(ItemSideMargin + uiThumbnailSize - 16 + 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
  thumbnailRect.setSize(QSize(16, 16));
  if (thumbnailRect.contains(pEvent->position().toPoint()))
  {
    pEvent->accept();
    return true;
  }
  return false;
}

bool xiiQtIconViewDelegate::mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& opt, const QModelIndex& index)
{
  const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)index.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
  if (!itemType.IsSet(xiiAssetBrowserItemFlags::Asset))
    return false;

  const xiiUInt32 uiThumbnailSize = ThumbnailSize();
  QRect           thumbnailRect   = opt.rect.adjusted(ItemSideMargin + uiThumbnailSize - 16 + 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
  thumbnailRect.setSize(QSize(16, 16));
  if (thumbnailRect.contains(pEvent->position().toPoint()))
  {
    xiiUuid guid = index.data(xiiQtAssetBrowserModel::UserRoles::AssetGuid).value<xiiUuid>();

    xiiTransformStatus ret = xiiAssetCurator::GetSingleton()->TransformAsset(guid, xiiTransformFlags::TriggeredManually);

    if (ret.Failed())
    {
      QString path = index.data(xiiQtAssetBrowserModel::UserRoles::RelativePath).toString();
      xiiLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, path.toUtf8().data());
    }
    else
    {
      xiiAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
    }

    pEvent->accept();
    return true;
  }
  return false;
}

QWidget* xiiQtIconViewDelegate::createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  xiiStringBuilder sAbsPath = index.data(xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().constData();

  QLineEdit* editor = new QLineEdit(pParent);
  editor->setValidator(new xiiFileNameValidator(editor, sAbsPath.GetFileDirectory(), sAbsPath.GetFileNameAndExtension()));
  return editor;
}

void xiiQtIconViewDelegate::setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const
{
  QString    sOldName  = index.data(xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
  QLineEdit* pLineEdit = qobject_cast<QLineEdit*>(pEditor);
  pModel->setData(index, pLineEdit->text());
}

void xiiQtIconViewDelegate::updateEditorGeometry(QWidget* pEditor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  if (!pEditor)
    return;

  const xiiUInt32 uiThumbnailSize = ThumbnailSize();
  const QRect     textRect        = option.rect.adjusted(ItemSideMargin, ItemSideMargin + uiThumbnailSize + TextSpacing, -ItemSideMargin, -ItemSideMargin - TextSpacing);
  pEditor->setGeometry(textRect);
}

void xiiQtIconViewDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  if (!IsInIconMode())
  {
    xiiQtItemDelegate::paint(pPainter, opt, index);
    return;
  }

  const xiiUInt32                             uiThumbnailSize = ThumbnailSize();
  const xiiBitflags<xiiAssetBrowserItemFlags> itemType        = (xiiAssetBrowserItemFlags::Enum)index.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

  // Prepare painter.
  {
    pPainter->save();
    if (hasClipping())
      pPainter->setClipRect(opt.rect);

    pPainter->setRenderHint(QPainter::SmoothPixmapTransform, true);
  }

  // Draw assets with a background to distinguish them easily from normal files / folders.
  if (itemType.IsAnySet(xiiAssetBrowserItemFlags::Asset | xiiAssetBrowserItemFlags::SubAsset))
  {
    QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
      cg = QPalette::Inactive;

    xiiInt32 border    = ItemSideMargin - HighlightBorderWidth;
    QRect    assetRect = opt.rect.adjusted(border, border, -border, -border);
    pPainter->fillRect(assetRect, opt.palette.brush(cg, QPalette::AlternateBase));
  }

  // Draw highlight background (copy of QItemDelegate::drawBackground)
  {
    QRect highlightRect = opt.rect.adjusted(ItemSideMargin - HighlightBorderWidth, ItemSideMargin - HighlightBorderWidth, 0, 0);
    highlightRect.setHeight(uiThumbnailSize + 2 * HighlightBorderWidth);
    highlightRect.setWidth(uiThumbnailSize + 2 * HighlightBorderWidth);

    if ((opt.state & QStyle::State_Selected))
    {
      QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
      if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
        cg = QPalette::Inactive;

      pPainter->fillRect(highlightRect, opt.palette.brush(cg, QPalette::Highlight));
    }
    else
    {
      QVariant value = index.data(Qt::BackgroundRole);
      if (value.canConvert<QBrush>())
      {
        QPointF oldBO = pPainter->brushOrigin();
        pPainter->setBrushOrigin(highlightRect.topLeft());
        pPainter->fillRect(highlightRect, qvariant_cast<QBrush>(value));
        pPainter->setBrushOrigin(oldBO);
      }
    }
  }

  if (itemType.IsAnySet(xiiAssetBrowserItemFlags::File) && !itemType.IsAnySet(xiiAssetBrowserItemFlags::Asset))
  {
    // Draw thumbnail.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin, 0, 0);
      thumbnailRect.setSize(QSize(uiThumbnailSize, uiThumbnailSize));
      QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
      icon.paint(pPainter, thumbnailRect);
    }

    // Draw icon.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin - 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
      thumbnailRect.setSize(QSize(16, 16));
      QIcon icon = qvariant_cast<QIcon>(index.data(xiiQtAssetBrowserModel::UserRoles::AssetIcon));
      icon.paint(pPainter, thumbnailRect);
    }
  }
  else if (itemType.IsAnySet(xiiAssetBrowserItemFlags::Folder | xiiAssetBrowserItemFlags::DataDirectory))
  {
    // Draw icon.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin, 0, 0);
      thumbnailRect.setSize(QSize(uiThumbnailSize, uiThumbnailSize));
      QIcon icon = qvariant_cast<QIcon>(index.data(xiiQtAssetBrowserModel::UserRoles::AssetIcon));
      icon.paint(pPainter, thumbnailRect);
    }
  }
  else // asset
  {
    // Draw thumbnail.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin, 0, 0);
      thumbnailRect.setSize(QSize(uiThumbnailSize, uiThumbnailSize));
      QPixmap pixmap = qvariant_cast<QPixmap>(index.data(Qt::DecorationRole));
      pPainter->drawPixmap(thumbnailRect, pixmap);
    }

    // Draw icon.
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin - 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
      thumbnailRect.setSize(QSize(16, 16));
      QIcon icon = qvariant_cast<QIcon>(index.data(xiiQtAssetBrowserModel::UserRoles::AssetIcon));
      icon.paint(pPainter, thumbnailRect);
    }

    // Draw Transform State Icon
    if (m_bDrawTransformState)
    {
      QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin + uiThumbnailSize - 16 + 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
      thumbnailRect.setSize(QSize(16, 16));

      xiiAssetInfo::TransformState state = (xiiAssetInfo::TransformState)index.data(xiiQtAssetBrowserModel::UserRoles::TransformState).toInt();

      switch (state)
      {
        case xiiAssetInfo::TransformState::Unknown:
          xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetUnknown.svg").paint(pPainter, thumbnailRect);
          break;
        case xiiAssetInfo::TransformState::NeedsThumbnail:
          xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsThumbnail.svg").paint(pPainter, thumbnailRect);
          break;
        case xiiAssetInfo::TransformState::NeedsTransform:
          xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsTransform.svg").paint(pPainter, thumbnailRect);
          break;
        case xiiAssetInfo::TransformState::UpToDate:
          xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetOk.svg").paint(pPainter, thumbnailRect);
          break;
        case xiiAssetInfo::TransformState::MissingTransformDependency:
          xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetMissingDependency.svg").paint(pPainter, thumbnailRect);
          break;
        case xiiAssetInfo::TransformState::MissingPackageDependency:
          xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetMissingDependency.svg").paint(pPainter, thumbnailRect);
          break;
        case xiiAssetInfo::TransformState::MissingThumbnailDependency:
          xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetMissingReference.svg").paint(pPainter, thumbnailRect);
          break;
        case xiiAssetInfo::TransformState::CircularDependency:
          xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetFailedTransform.svg").paint(pPainter, thumbnailRect);
          break;
        case xiiAssetInfo::TransformState::TransformError:
          xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetFailedTransform.svg").paint(pPainter, thumbnailRect);
          break;
        case xiiAssetInfo::TransformState::NeedsImport:
          xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsImport.svg").paint(pPainter, thumbnailRect);
          break;
        case xiiAssetInfo::TransformState::COUNT:
          break;
      }
    }
  }

  // Draw caption.
  {
    pPainter->setFont(GetFont());
    QRect textRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin + uiThumbnailSize + TextSpacing, -ItemSideMargin, -ItemSideMargin - TextSpacing);

    QString caption = qvariant_cast<QString>(index.data(Qt::DisplayRole));
    pPainter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWrapAnywhere, caption);
  }


  pPainter->restore();
}

QSize xiiQtIconViewDelegate::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  if (IsInIconMode())
  {
    return ItemSize();
  }
  else
  {
    return xiiQtItemDelegate::sizeHint(opt, index);
  }
}

QSize xiiQtIconViewDelegate::ItemSize() const
{
  QFont        font = GetFont();
  QFontMetrics fm(font);

  xiiUInt32       iThumbnail  = ThumbnailSize();
  const xiiUInt32 iItemWidth  = iThumbnail + 2 * ItemSideMargin;
  const xiiUInt32 iItemHeight = iThumbnail + 2 * (ItemSideMargin + fm.height() + TextSpacing);

  return QSize(iItemWidth, iItemHeight);
}

QFont xiiQtIconViewDelegate::GetFont() const
{
  QFont font = QApplication::font();

  float fScaleFactor = xiiMath::Clamp((1.0f + (m_iIconSizePercentage / 100.0f)) * 0.75f, 0.75f, 1.25f);

  font.setPointSizeF(font.pointSizeF() * fScaleFactor);
  return font;
}

xiiUInt32 xiiQtIconViewDelegate::ThumbnailSize() const
{
  return static_cast<xiiUInt32>((float)MaxSize * (float)m_iIconSizePercentage / 100.0f);
}

bool xiiQtIconViewDelegate::IsInIconMode() const
{
  return m_pView->viewMode() == QListView::ViewMode::IconMode;
}
