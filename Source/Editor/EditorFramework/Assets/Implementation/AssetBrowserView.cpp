#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetBrowserView.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>


xiiQtAssetBrowserView::xiiQtAssetBrowserView(QWidget* parent) :
  xiiQtItemView<QListView>(parent)
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

void xiiQtAssetBrowserView::SetDialogMode(bool bDialogMode)
{
  m_bDialogMode = bDialogMode;

  if (m_bDialogMode)
  {
    m_pDelegate->SetDrawTransformState(false);
    setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
    setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  }
  else
  {
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

bool xiiQtIconViewDelegate::mousePressEvent(QMouseEvent* event, const QStyleOptionViewItem& opt, const QModelIndex& index)
{
  const xiiUInt32 uiThumbnailSize = ThumbnailSize();
  QRect           thumbnailRect   = opt.rect.adjusted(ItemSideMargin + uiThumbnailSize - 16 + 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
  thumbnailRect.setSize(QSize(16, 16));
  if (thumbnailRect.contains(event->localPos().toPoint()))
  {
    event->accept();
    return true;
  }
  return false;
}

bool xiiQtIconViewDelegate::mouseReleaseEvent(QMouseEvent* event, const QStyleOptionViewItem& opt, const QModelIndex& index)
{
  const xiiUInt32 uiThumbnailSize = ThumbnailSize();
  QRect           thumbnailRect   = opt.rect.adjusted(ItemSideMargin + uiThumbnailSize - 16 + 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
  thumbnailRect.setSize(QSize(16, 16));
  if (thumbnailRect.contains(event->localPos().toPoint()))
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

    event->accept();
    return true;
  }
  return false;
}

void xiiQtIconViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  if (!IsInIconMode())
  {
    xiiQtItemDelegate::paint(painter, opt, index);
    return;
  }

  const xiiUInt32 uiThumbnailSize = ThumbnailSize();

  // Prepare painter.
  {
    painter->save();
    if (hasClipping())
      painter->setClipRect(opt.rect);

    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
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

      painter->fillRect(highlightRect, opt.palette.brush(cg, QPalette::Highlight));
    }
    else
    {
      QVariant value = index.data(Qt::BackgroundRole);
      if (value.canConvert<QBrush>())
      {
        QPointF oldBO = painter->brushOrigin();
        painter->setBrushOrigin(highlightRect.topLeft());
        painter->fillRect(highlightRect, qvariant_cast<QBrush>(value));
        painter->setBrushOrigin(oldBO);
      }
    }
  }

  // Draw thumbnail.
  {
    QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin, 0, 0);
    thumbnailRect.setSize(QSize(uiThumbnailSize, uiThumbnailSize));
    QPixmap pixmap = qvariant_cast<QPixmap>(index.data(Qt::DecorationRole));
    painter->drawPixmap(thumbnailRect, pixmap);
  }

  // Draw icon.
  {
    QRect thumbnailRect = opt.rect.adjusted(ItemSideMargin - 2, ItemSideMargin + uiThumbnailSize - 16 + 2, 0, 0);
    thumbnailRect.setSize(QSize(16, 16));
    QIcon icon = qvariant_cast<QIcon>(index.data(xiiQtAssetBrowserModel::UserRoles::AssetIconPath));
    icon.paint(painter, thumbnailRect);
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
        xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetUnknown16.png").paint(painter, thumbnailRect);
        break;
      case xiiAssetInfo::TransformState::NeedsThumbnail:
        xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsThumbnail16.png").paint(painter, thumbnailRect);
        break;
      case xiiAssetInfo::TransformState::NeedsTransform:
        xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsTransform16.png").paint(painter, thumbnailRect);
        break;
      case xiiAssetInfo::TransformState::UpToDate:
        xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetOk16.png").paint(painter, thumbnailRect);
        break;
      case xiiAssetInfo::TransformState::MissingDependency:
        xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetMissingDependency16.png").paint(painter, thumbnailRect);
        break;
      case xiiAssetInfo::TransformState::MissingReference:
        xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetMissingReference16.png").paint(painter, thumbnailRect);
        break;
      case xiiAssetInfo::TransformState::TransformError:
        xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetFailedTransform16.png").paint(painter, thumbnailRect);
        break;
      case xiiAssetInfo::TransformState::NeedsImport:
        xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetNeedsImport16.png").paint(painter, thumbnailRect);
        break;
      case xiiAssetInfo::TransformState::COUNT:
        break;
    }
  }

  // Draw caption.
  {
    painter->setFont(GetFont());
    QRect textRect = opt.rect.adjusted(ItemSideMargin, ItemSideMargin + uiThumbnailSize + TextSpacing, -ItemSideMargin, -ItemSideMargin - TextSpacing);

    QString caption = qvariant_cast<QString>(index.data(Qt::DisplayRole));
    painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWrapAnywhere, caption);
  }


  painter->restore();
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
  return MaxSize * (float)m_iIconSizePercentage / 100.0f;
}

bool xiiQtIconViewDelegate::IsInIconMode() const
{
  return m_pView->viewMode() == QListView::ViewMode::IconMode;
}
