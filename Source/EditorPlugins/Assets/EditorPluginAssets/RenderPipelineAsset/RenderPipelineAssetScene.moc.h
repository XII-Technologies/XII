/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

protected:
  void ExtendContextMenu(QMenu& menu, xiiQtNode* pNode) override;
};
