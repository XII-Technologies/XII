#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>

xiiQtScenegraphModel::xiiQtScenegraphModel(const xiiDocumentObjectManager* pObjectManager, const xiiUuid& root) :
  xiiQtGameObjectModel(pObjectManager, root)
{
}

xiiQtScenegraphModel::~xiiQtScenegraphModel() = default;
