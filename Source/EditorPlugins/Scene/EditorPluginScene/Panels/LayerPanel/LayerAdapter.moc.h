/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>

#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>

class xiiScene2Document;
struct xiiScene2LayerEvent;
struct xiiDocumentEvent;

/// Custom adapter for layers, used in xiiQtLayerPanel.
class XII_EDITORPLUGINSCENE_DLL xiiQtLayerAdapter : public xiiQtDocumentTreeModelAdapter
{
  Q_OBJECT;

public:
  xiiQtLayerAdapter(xiiScene2Document* pDocument);
  ~xiiQtLayerAdapter();
  virtual QVariant data(const xiiDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;
  virtual bool     setData(const xiiDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const override;

  enum UserRoles
  {
    LayerGuid = Qt::UserRole + 0,
  };

private:
  void LayerEventHandler(const xiiScene2LayerEvent& e);
  void DocumentEventHander(const xiiDocumentEvent& e);

private:
  xiiScene2Document*                                 m_pSceneDocument;
  xiiEvent<const xiiScene2LayerEvent&>::Unsubscriber m_LayerEventUnsubscriber;
  xiiEvent<const xiiDocumentEvent&>::Unsubscriber    m_DocumentEventUnsubscriber;
  xiiUuid                                            m_CurrentActiveLayer;
};

/// Custom delegate for layers, used in xiiQtLayerPanel.
/// Provides buttons to toggle the layer visible / loaded states.
/// Relies on xiiQtLayerAdapter to trigger updates and provide the LayerGuid.
class xiiQtLayerDelegate : public xiiQtItemDelegate
{
  Q_OBJECT
public:
  xiiQtLayerDelegate(QObject* pParent, xiiScene2Document* pDocument);

  virtual bool  mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool  mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool  mouseMoveEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual void  paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual QSize sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual bool  helpEvent(QHelpEvent* pEvent, QAbstractItemView* pView, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  static QRect  GetVisibleIconRect(const QStyleOptionViewItem& opt);
  static QRect  GetLoadedIconRect(const QStyleOptionViewItem& opt);

  bool               m_bPressed  = false;
  xiiScene2Document* m_pDocument = nullptr;
};

/// Custom model for layers, used in xiiQtLayerPanel.
class xiiQtLayerModel : public xiiQtDocumentTreeModel
{
  Q_OBJECT

public:
  xiiQtLayerModel(xiiScene2Document* pDocument);
  ~xiiQtLayerModel() = default;

private:
  xiiScene2Document* m_pDocument = nullptr;
};
