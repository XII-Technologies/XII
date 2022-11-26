#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <QGraphicsPathItem>

class xiiPin;
class xiiQtConnection;

enum class xiiQtPinHighlightState
{
  None,
  CannotConnect,
  CannotConnectSameDirection,
  CanAddConnection,
  CanReplaceConnection,
};

class XII_GUIFOUNDATION_DLL xiiQtPin : public QGraphicsPathItem
{
public:
  xiiQtPin();
  ~xiiQtPin();
  virtual int type() const override { return xiiQtNodeScene::Pin; }

  void                          AddConnection(xiiQtConnection* pConnection);
  void                          RemoveConnection(xiiQtConnection* pConnection);
  xiiArrayPtr<xiiQtConnection*> GetConnections() { return m_Connections; }
  bool                          HasAnyConnections() const { return !m_Connections.IsEmpty(); }

  const xiiPin* GetPin() const { return m_pPin; }
  virtual void  SetPin(const xiiPin& pin);
  virtual void  ConnectedStateChanged(bool bConnected);

  virtual QPointF GetPinPos() const;
  virtual QPointF GetPinDir() const;
  virtual QRectF  GetPinRect() const;
  virtual void    UpdateConnections();
  void            SetHighlightState(xiiQtPinHighlightState state);

  void SetActive(bool active);

  virtual void ExtendContextMenu(QMenu& menu) {}
  virtual void keyPressEvent(QKeyEvent* event) override {}

protected:
  virtual bool     AdjustRenderingForHighlight(xiiQtPinHighlightState state);
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  xiiQtPinHighlightState m_HighlightState = xiiQtPinHighlightState::None;
  QGraphicsTextItem*     m_pLabel;
  QPointF                m_PinCenter;

private:
  bool m_bIsActive = true;

  const xiiPin*                       m_pPin = nullptr;
  xiiHybridArray<xiiQtConnection*, 6> m_Connections;
};
