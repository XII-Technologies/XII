#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class xiiDecalComponentDragDropHandler : public xiiComponentDragDropHandler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalComponentDragDropHandler, xiiComponentDragDropHandler);

public:
  virtual float CanHandle(const xiiDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const xiiDragDropInfo* pInfo) override;
};
