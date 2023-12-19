#pragma once

#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class xiiSceneDocument;

class xiiQtScenegraphModel : public xiiQtGameObjectModel
{
  Q_OBJECT

public:
  xiiQtScenegraphModel(const xiiDocumentObjectManager* pObjectManager, const xiiUuid& root = xiiUuid());
  ~xiiQtScenegraphModel();
};
