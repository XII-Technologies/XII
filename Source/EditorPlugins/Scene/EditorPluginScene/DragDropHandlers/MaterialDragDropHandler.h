/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/DragDrop/AssetDragDropHandler.h>

class xiiMaterialDragDropHandler : public xiiAssetDragDropHandler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMaterialDragDropHandler, xiiAssetDragDropHandler);

public:
protected:
  virtual void  RequestConfiguration(xiiDragDropConfig* pConfigToFillOut) override;
  virtual float CanHandle(const xiiDragDropInfo* pInfo) const override;
  virtual void  OnDragBegin(const xiiDragDropInfo* pInfo) override;
  virtual void  OnDragUpdate(const xiiDragDropInfo* pInfo) override;
  virtual void  OnDragCancel() override;
  virtual void  OnDrop(const xiiDragDropInfo* pInfo) override;

  xiiUuid  m_AppliedToComponent;
  xiiInt32 m_iAppliedToSlot;
};
