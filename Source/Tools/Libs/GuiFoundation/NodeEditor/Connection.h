#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <QGraphicsPathItem>

class xiiPin;

class XII_GUIFOUNDATION_DLL xiiQtConnection : public QGraphicsPathItem
{
public:
  explicit xiiQtConnection(QGraphicsItem* pParent = 0);
  ~xiiQtConnection();
  virtual int type() const override { return xiiQtNodeScene::Connection; }

  const xiiDocumentObject* GetObject() const { return m_pObject; }
  const xiiConnection*     GetConnection() const { return m_pConnection; }
  void                     InitConnection(const xiiDocumentObject* pObject, const xiiConnection* pConnection);

  void SetPosIn(const QPointF& point);
  void SetPosOut(const QPointF& point);
  void SetDirIn(const QPointF& dir);
  void SetDirOut(const QPointF& dir);

  virtual void UpdateGeometry();
  virtual QPen DeterminePen() const;

  const QPointF& GetInPos() const { return m_InPoint; }
  const QPointF& GetOutPos() const { return m_OutPoint; }

  bool m_bAdjacentNodeSelected = false;

  virtual void ExtendContextMenu(QMenu& ref_menu) {}

protected:
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  // Draws connections following the rules of subway maps (angles of 45 degrees only).
  void DrawSubwayPath(QPainterPath& path, const QPointF& startPoint, const QPointF& endPoint);

  const xiiDocumentObject* m_pObject     = nullptr;
  const xiiConnection*     m_pConnection = nullptr;

  QPointF m_InPoint;
  QPointF m_OutPoint;
  QPointF m_InDir;
  QPointF m_OutDir;
};
