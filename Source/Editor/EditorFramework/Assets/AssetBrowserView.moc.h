#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetBrowserFolderView.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>

#include <QItemDelegate>
#include <QListView>

class xiiQtIconViewDelegate;

class xiiQtAssetBrowserView : public xiiQtItemView<QListView>
{
  Q_OBJECT

public:
  xiiQtAssetBrowserView(QWidget* pParent);
  void SetDialogMode(bool bDialogMode);

  void     SetIconMode(bool bIconMode);
  void     SetIconScale(xiiInt32 iIconSizePercentage);
  xiiInt32 GetIconScale() const;

  void dragEnterEvent(QDragEnterEvent* pEvent) override;
  void dragMoveEvent(QDragMoveEvent* pEvent) override;
  void dragLeaveEvent(QDragLeaveEvent* pEvent) override;
  void dropEvent(QDropEvent* pEvent) override;
  void startDrag(Qt::DropActions supportedActions) override;

Q_SIGNALS:
  void ViewZoomed(xiiInt32 iIconSizePercentage);

protected:
  virtual void wheelEvent(QWheelEvent* pEvent) override;
  virtual void mousePressEvent(QMouseEvent* pEvent) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* pEvent) override;
  virtual void mouseMoveEvent(QMouseEvent* pEvent) override;

private:
  bool                   m_bDialogMode;
  xiiQtIconViewDelegate* m_pDelegate;
  xiiInt32               m_iIconSizePercentage;
};


class xiiQtIconViewDelegate : public xiiQtItemDelegate
{
  Q_OBJECT

public:
  xiiQtIconViewDelegate(xiiQtAssetBrowserView* pParent = nullptr);

  void SetDrawTransformState(bool b) { m_bDrawTransformState = b; }

  void SetIconScale(xiiInt32 iIconSizePercentage);

  virtual bool mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;


public:
  virtual void     paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual QSize    sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual QWidget* createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  virtual void     setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const override;
  virtual void     updateEditorGeometry(QWidget* pEditor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
  QSize     ItemSize() const;
  QFont     GetFont() const;
  xiiUInt32 ThumbnailSize() const;
  bool      IsInIconMode() const;

private:
  enum
  {
    MaxSize              = xiiThumbnailSize,
    HighlightBorderWidth = 3,
    ItemSideMargin       = 5,
    TextSpacing          = 5
  };

  bool                   m_bDrawTransformState;
  xiiInt32               m_iIconSizePercentage;
  xiiQtAssetBrowserView* m_pView;
};
