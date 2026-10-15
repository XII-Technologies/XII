/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetScene.moc.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>

xiiQtRenderPipelineAssetScene::xiiQtRenderPipelineAssetScene(QObject* pParent) :
  xiiQtNodeScene(pParent)
{
}

xiiQtRenderPipelineAssetScene::~xiiQtRenderPipelineAssetScene() = default;

void xiiQtRenderPipelineAssetScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
  for (auto* item : items(event->scenePos()))
  {
    while (item && item->type() != xiiQtNodeScene::Node)
      item = item->parentItem();
    if (!item)
      continue;
    const auto* object = static_cast<xiiQtNode*>(item)->GetObject();
    if (!object->GetType()->IsDerivedFrom<xiiRenderPipelineSubgraphPass>())
      break;
    QMenu menu;
    auto* expand = menu.addAction("Expand Subgraph");
    if (menu.exec(event->screenPos()) == expand)
    {
      auto* document = const_cast<xiiRenderPipelineAssetDocument*>(static_cast<const xiiRenderPipelineAssetDocument*>(GetDocumentNodeManager()->GetDocument()));
      xiiQtUiServices::GetSingleton()->MessageBoxStatus(document->ExpandSubgraph(object), "Could not expand render pipeline subgraph.");
    }
    event->accept();
    return;
  }
  xiiQtNodeScene::contextMenuEvent(event);
}
