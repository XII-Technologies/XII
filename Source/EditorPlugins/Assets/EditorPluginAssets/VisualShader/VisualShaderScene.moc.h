/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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
  xiiQtVisualShaderScene(QObject* pParent = nullptr);
  ~xiiQtVisualShaderScene();
};

class xiiQtVisualShaderPin : public xiiQtPin
{
public:
  xiiQtVisualShaderPin();

  virtual void SetPin(const xiiPin& pin) override;
  virtual void paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;
};

class xiiQtVisualShaderNode : public xiiQtNode
{
public:
  xiiQtVisualShaderNode();

  virtual void InitNode(const xiiDocumentNodeManager* pManager, const xiiDocumentObject* pObject) override;

  virtual void UpdateState() override;
};
