#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>

class xiiQtNodeScene;
class xiiQtNodeView;

class xiiQtRenderPipelineAssetScene : public xiiQtNodeScene
{
  Q_OBJECT

public:
  xiiQtRenderPipelineAssetScene(QObject* pParent = nullptr);
  ~xiiQtRenderPipelineAssetScene();
};
