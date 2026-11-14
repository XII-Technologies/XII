/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>

class xiiScene2Document;

class xiiQtScene2DocumentWindow : public xiiQtSceneDocumentWindowBase
{
  Q_OBJECT

public:
  xiiQtScene2DocumentWindow(xiiScene2Document* pDocument);
  ~xiiQtScene2DocumentWindow() override;

  xiiStringView GetWindowLayoutGroupName() const override { return "Scene2"; }
  bool          InternalCanCloseWindow() override;

  xiiStatus SaveAllLayers();
};
