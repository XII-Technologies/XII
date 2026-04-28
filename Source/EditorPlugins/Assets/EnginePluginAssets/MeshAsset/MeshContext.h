/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Meshes/MeshResource.h>

class xiiObjectSelectionMsgToEngine;

class XII_ENGINEPLUGINASSETS_DLL xiiMeshContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshContext, xiiEngineProcessDocumentContext);

public:
  xiiMeshContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

  const xiiMeshResourceHandle& GetMesh() const { return m_hMesh; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;
  virtual bool                         UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg);
  void OnResourceEvent(const xiiResourceEvent& e);

  xiiGameObject*        m_pMeshObject;
  xiiMeshResourceHandle m_hMesh;

  xiiAtomicBool                                             m_bBoundsDirty = false;
  xiiEvent<const xiiResourceEvent&, xiiMutex>::Unsubscriber m_MeshResourceEventSubscriber;
};
