#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class xiiVisualScriptComponentDragDropHandler : public xiiComponentDragDropHandler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptComponentDragDropHandler, xiiComponentDragDropHandler);

public:
  virtual float CanHandle(const xiiDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const xiiDragDropInfo* pInfo) override;
};
