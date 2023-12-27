#pragma once

#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class xiiQtStateMachinePin : public xiiQtPin
{
public:
  xiiQtStateMachinePin();

  virtual void   SetPin(const xiiPin& pin) override;
  virtual QRectF GetPinRect() const override;
};

class xiiQtStateMachineConnection : public xiiQtConnection
{
public:
  xiiQtStateMachineConnection();
};

class xiiQtStateMachineNode : public xiiQtNode
{
public:
  xiiQtStateMachineNode();

  virtual void InitNode(const xiiDocumentNodeManager* pManager, const xiiDocumentObject* pObject) override;
  virtual void UpdateGeometry() override;
  virtual void UpdateState() override;
  virtual void ExtendContextMenu(QMenu& ref_menu) override;

  bool IsInitialState() const;
  bool IsAnyState() const;

private:
  void UpdateHeaderColor();
};

class xiiQtStateMachineAssetScene : public xiiQtNodeScene
{
  Q_OBJECT

public:
  xiiQtStateMachineAssetScene(QObject* pParent = nullptr);
  ~xiiQtStateMachineAssetScene();

  void SetInitialState(xiiQtStateMachineNode* pNode);

private:
  virtual xiiStatus RemoveNode(xiiQtNode* pNode) override;
};
