#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class xiiJoltCollisionMeshComponentDragDropHandler : public xiiComponentDragDropHandler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltCollisionMeshComponentDragDropHandler, xiiComponentDragDropHandler);

public:
  virtual float CanHandle(const xiiDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const xiiDragDropInfo* pInfo) override;
};
