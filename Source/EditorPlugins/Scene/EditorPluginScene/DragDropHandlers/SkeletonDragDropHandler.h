#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class xiiSkeletonComponentDragDropHandler : public xiiComponentDragDropHandler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkeletonComponentDragDropHandler, xiiComponentDragDropHandler);

public:
  virtual float CanHandle(const xiiDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const xiiDragDropInfo* pInfo) override;
};
