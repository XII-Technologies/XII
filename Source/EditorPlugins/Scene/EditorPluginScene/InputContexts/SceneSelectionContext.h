/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/InputContexts/SelectionContext.h>

/// Custom selection context for the scene to allow switching the active layer if an object is clicked that is in a different layer then the active one.
class xiiSceneSelectionContext : public xiiSelectionContext
{
public:
  xiiSceneSelectionContext(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView, const xiiCamera* pCamera);

protected:
  virtual void OpenDocumentForPickedObject(const xiiObjectPickingResult& res) const override;
  virtual void SelectPickedObject(const xiiObjectPickingResult& res, bool bToggle, bool bDirect) const override;

  xiiUuid FindLayerByObject(xiiUuid objectGuid, const xiiDocumentObject*& out_pObject) const;
};
