/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetScene.moc.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMenu>

xiiQtRenderPipelineAssetScene::xiiQtRenderPipelineAssetScene(QObject* pParent) :
  xiiQtNodeScene(pParent)
{
}

xiiQtRenderPipelineAssetScene::~xiiQtRenderPipelineAssetScene() = default;

void xiiQtRenderPipelineAssetScene::ExtendContextMenu(QMenu& menu, xiiQtNode* pNode)
{
  if (!pNode || !pNode->GetObject()->GetType()->IsDerivedFrom<xiiRenderPipelineSubgraphPass>())
    return;
  menu.addSeparator();
  connect(menu.addAction("Expand Subgraph"), &QAction::triggered, this, [this, object = pNode->GetObject()]() {
    auto* document = const_cast<xiiRenderPipelineAssetDocument*>(static_cast<const xiiRenderPipelineAssetDocument*>(GetDocumentNodeManager()->GetDocument()));
    xiiQtUiServices::GetSingleton()->MessageBoxStatus(document->ExpandSubgraph(object), "Could not expand render pipeline subgraph.");
  });
}
