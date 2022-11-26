#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class xiiQtProcGenNode : public xiiQtNode
{
public:
  xiiQtProcGenNode();

  virtual void InitNode(const xiiDocumentNodeManager* pManager, const xiiDocumentObject* pObject) override;

  virtual void UpdateState() override;
};

class xiiQtProcGenPin : public xiiQtPin
{
public:
  xiiQtProcGenPin();
  ~xiiQtProcGenPin();

  virtual void ExtendContextMenu(QMenu& menu) override;

  virtual void   keyPressEvent(QKeyEvent* event) override;
  virtual void   paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  virtual QRectF boundingRect() const override;

  void SetDebug(bool bDebug);

private:
  bool m_bDebug = false;
};

class xiiQtProcGenScene : public xiiQtNodeScene
{
public:
  xiiQtProcGenScene(QObject* parent = nullptr);
  ~xiiQtProcGenScene();

  void SetDebugPin(xiiQtProcGenPin* pDebugPin);

private:
  virtual xiiStatus RemoveNode(xiiQtNode* pNode) override;

  bool             m_bUpdatingDebugPin = false;
  xiiQtProcGenPin* m_pDebugPin         = nullptr;
};
