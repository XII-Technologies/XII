/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

/// Base class for drag and drop handler that drop on a xiiSceneLayer.
class xiiLayerDragDropHandler : public xiiDragDropHandler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLayerDragDropHandler, xiiDragDropHandler);

public:
  virtual void OnDragBegin(const xiiDragDropInfo* pInfo) override {}
  virtual void OnDragUpdate(const xiiDragDropInfo* pInfo) override {}
  virtual void OnDragCancel() override {}

protected:
  const xiiRTTI* GetCommonBaseType(const xiiDragDropInfo* pInfo) const;
};

class xiiLayerOnLayerDragDropHandler : public xiiLayerDragDropHandler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLayerOnLayerDragDropHandler, xiiLayerDragDropHandler);

public:
  virtual float CanHandle(const xiiDragDropInfo* pInfo) const override;
  virtual void  OnDrop(const xiiDragDropInfo* pInfo) override;
};

class xiiGameObjectOnLayerDragDropHandler : public xiiLayerDragDropHandler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameObjectOnLayerDragDropHandler, xiiLayerDragDropHandler);

public:
  virtual float CanHandle(const xiiDragDropInfo* pInfo) const override;
  virtual void  OnDrop(const xiiDragDropInfo* pInfo) override;
};
