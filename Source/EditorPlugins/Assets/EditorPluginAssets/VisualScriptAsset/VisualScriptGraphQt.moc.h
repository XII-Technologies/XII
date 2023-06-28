#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class xiiQtNodeView;
struct xiiVisualScriptActivityEvent;
struct xiiVisualScriptInstanceActivity;

class xiiQtVisualScriptAssetScene : public xiiQtNodeScene
{
  Q_OBJECT

public:
  xiiQtVisualScriptAssetScene(QObject* pParent = nullptr);
  ~xiiQtVisualScriptAssetScene();

  void    VisualScriptActivityEventHandler(const xiiVisualScriptActivityEvent& ae);
  void    VisualScriptInterDocumentMessageHandler(xiiReflectedClass* pMsg);
  void    SetDebugObject(const xiiUuid& objectGuid);
  xiiUuid GetDebugObject() const { return m_DebugObject; }

private Q_SLOTS:
  void OnUpdateDisplay();

private:
  void GetAllVsNodes(xiiDynamicArray<const xiiDocumentObject*>& allNodes) const;
  void ResetActiveConnections(xiiDynamicArray<const xiiDocumentObject*>& allNodes);

  xiiUuid m_DebugObject;
};

class xiiQtVisualScriptPin_Legacy : public xiiQtPin
{
public:
  xiiQtVisualScriptPin_Legacy();

  virtual void SetPin(const xiiPin& pin) override;
};

class xiiQtVisualScriptConnection_Legacy : public xiiQtConnection
{
public:
  xiiQtVisualScriptConnection_Legacy(QGraphicsItem* parent = 0);

  virtual QPen DeterminePen() const override;

  bool    m_bExecutionHighlight = false;
  xiiTime m_HighlightUntil;
};

class xiiQtVisualScriptNode_Legacy : public xiiQtNode
{
public:
  xiiQtVisualScriptNode_Legacy();

  virtual void InitNode(const xiiDocumentNodeManager* pManager, const xiiDocumentObject* pObject) override;

  virtual void UpdateState() override;
};
