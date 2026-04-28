/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>

class xiiQtNodeScene;
class xiiQtNodeView;

class xiiQtAnimationGraphAssetScene : public xiiQtNodeScene
{
  Q_OBJECT

public:
  xiiQtAnimationGraphAssetScene(QObject* pParent = nullptr);
  ~xiiQtAnimationGraphAssetScene();
};
