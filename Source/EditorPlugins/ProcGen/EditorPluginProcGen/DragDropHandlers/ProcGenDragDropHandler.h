#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>
#include <EditorPluginProcGen/EditorPluginProcGenDLL.h>

class XII_EDITORPLUGINPROCGEN_DLL xiiProcPlacementComponentDragDropHandler : public xiiComponentDragDropHandler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcPlacementComponentDragDropHandler, xiiComponentDragDropHandler);

public:
  virtual float CanHandle(const xiiDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const xiiDragDropInfo* pInfo) override;
};
