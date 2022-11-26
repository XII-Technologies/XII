#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class xiiQtNodeView;

class xiiQtVisualShaderScene : public xiiQtNodeScene
{
  Q_OBJECT

public:
  xiiQtVisualShaderScene(QObject* parent = nullptr);
  ~xiiQtVisualShaderScene();
};

class xiiQtVisualShaderPin : public xiiQtPin
{
public:
  xiiQtVisualShaderPin();

  virtual void SetPin(const xiiPin& pin) override;
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};

class xiiQtVisualShaderNode : public xiiQtNode
{
public:
  xiiQtVisualShaderNode();

  virtual void InitNode(const xiiDocumentNodeManager* pManager, const xiiDocumentObject* pObject) override;

  virtual void UpdateState() override;
};
