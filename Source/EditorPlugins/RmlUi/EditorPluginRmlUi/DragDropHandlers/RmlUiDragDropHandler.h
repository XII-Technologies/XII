#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class xiiRmlUiComponentDragDropHandler : public xiiComponentDragDropHandler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRmlUiComponentDragDropHandler, xiiComponentDragDropHandler);

public:
  float CanHandle(const xiiDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const xiiDragDropInfo* pInfo) override;
};
