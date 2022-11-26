#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

//class xiiQtNodeView;

//class xiiQtVisualScriptAssetScene : public xiiQtNodeScene
//{
//  Q_OBJECT
//
//public:
//  xiiQtVisualScriptAssetScene(QObject* parent = nullptr);
//  ~xiiQtVisualScriptAssetScene();
//
//  void VisualScriptActivityEventHandler(const xiiVisualScriptActivityEvent& ae);
//  void VisualScriptInterDocumentMessageHandler(xiiReflectedClass* pMsg);
//  void SetDebugObject(const xiiUuid& objectGuid);
//  xiiUuid GetDebugObject() const { return m_DebugObject; }
//
//private Q_SLOTS:
//  void OnUpdateDisplay();
//
//private:
//  void GetAllVsNodes(xiiDynamicArray<const xiiDocumentObject*>& allNodes) const;
//  void ResetActiveConnections(xiiDynamicArray<const xiiDocumentObject*>& allNodes);
//
//  xiiUuid m_DebugObject;
//};

//class xiiQtVisualScriptPin : public xiiQtPin
//{
//public:
//  xiiQtVisualScriptPin();
//
//  virtual void SetPin(const xiiPin* pPin) override;
//  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
//};
//
//class xiiQtVisualScriptConnection : public xiiQtConnection
//{
//public:
//  xiiQtVisualScriptConnection(QGraphicsItem* parent = 0);
//
//  virtual QPen DeterminePen() const override;
//
//  bool m_bExecutionHighlight = false;
//  xiiTime m_HighlightUntil;
//};

class xiiQtAnimationControllerNode : public xiiQtNode
{
public:
  xiiQtAnimationControllerNode();

  virtual void UpdateState() override;
};
