/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <QGraphicsWidget>

// Avoid conflicts with windows.
#ifdef GetObject
#  undef GetObject
#endif

class xiiQtPin;
class xiiDocumentNodeManager;
class QLabel;
class xiiDocumentObject;
class QGraphicsTextItem;
class QGraphicsPixmapItem;
class QGraphicsDropShadowEffect;

struct xiiNodeFlags
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    None        = 0,
    Moved       = XII_BIT(0),
    UpdateTitle = XII_BIT(1),
    Default     = None
  };

  struct Bits
  {
    StorageType Moved : 1;
    StorageType UpdateTitle : 1;
  };
};

class XII_GUIFOUNDATION_DLL xiiQtNode : public QGraphicsPathItem
{
public:
  xiiQtNode();
  ~xiiQtNode();
  virtual int type() const override { return xiiQtNodeScene::Node; }

  const xiiDocumentObject* GetObject() const { return m_pObject; }
  virtual void             InitNode(const xiiDocumentNodeManager* pManager, const xiiDocumentObject* pObject);

  virtual void UpdateGeometry();

  void CreatePins();

  xiiQtPin* GetInputPin(const xiiPin& pin);
  xiiQtPin* GetOutputPin(const xiiPin& pin);

  xiiBitflags<xiiNodeFlags> GetFlags() const;
  void                      ResetFlags();

  void         EnableDropShadow(bool bEnable);
  virtual void UpdateState();

  const xiiHybridArray<xiiQtPin*, 6>& GetInputPins() const { return m_Inputs; }
  const xiiHybridArray<xiiQtPin*, 6>& GetOutputPins() const { return m_Outputs; }

  void SetActive(bool bActive);

  virtual void ExtendContextMenu(QMenu& ref_menu) {}

protected:
  virtual void     paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  QColor               m_HeaderColor;
  QRectF               m_HeaderRect;
  QGraphicsTextItem*   m_pTitleLabel    = nullptr;
  QGraphicsTextItem*   m_pSubtitleLabel = nullptr;
  QGraphicsPixmapItem* m_pIcon          = nullptr;

private:
  const xiiDocumentNodeManager* m_pManager = nullptr;
  const xiiDocumentObject*      m_pObject  = nullptr;
  xiiBitflags<xiiNodeFlags>     m_DirtyFlags;

  bool m_bIsActive = true;

  QGraphicsDropShadowEffect* m_pShadow = nullptr;

  // Pins
  xiiHybridArray<xiiQtPin*, 6> m_Inputs;
  xiiHybridArray<xiiQtPin*, 6> m_Outputs;
};
