/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class xiiPrefabComponentDragDropHandler : public xiiComponentDragDropHandler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPrefabComponentDragDropHandler, xiiComponentDragDropHandler);

protected:
  virtual float CanHandle(const xiiDragDropInfo* pInfo) const override;
  virtual void  OnDragBegin(const xiiDragDropInfo* pInfo) override;
  virtual void  OnDragUpdate(const xiiDragDropInfo* pInfo) override;

private:
  void CreatePrefab(const xiiVec3& vPosition, const xiiUuid& AssetGuid, xiiUuid parent, xiiInt32 iInsertChildIndex);
};
