#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>

class xiiQtNodeScene;
class xiiQtNodeView;

class xiiQtAnimationControllerAssetScene : public xiiQtNodeScene
{
  Q_OBJECT

public:
  xiiQtAnimationControllerAssetScene(QObject* parent = nullptr);
  ~xiiQtAnimationControllerAssetScene();
};
