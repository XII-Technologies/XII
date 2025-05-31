#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Meshes/MeshResource.h>

class xiiObjectSelectionMsgToEngine;
|
class XII_ENGINEPLUGINASSETS_DLL xiiAnimatedMeshContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimatedMeshContext, xiiEngineProcessDocumentContext);

public:
  xiiAnimatedMeshContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

  const xiiMeshResourceHandle& GetAnimatedMesh() const { return m_hAnimatedMesh; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;
  virtual bool                         UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg);

  xiiGameObject*        m_pAnimatedMeshObject;
  xiiMeshResourceHandle m_hAnimatedMesh;
};
