#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class xiiQtAnimationGraphNode : public xiiQtNode
{
public:
  xiiQtAnimationGraphNode();

  virtual void UpdateState() override;
};
