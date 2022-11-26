#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginJolt/EnginePluginJoltDLL.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>

class xiiObjectSelectionMsgToEngine;
class xiiRenderContext;

class XII_ENGINEPLUGINJOLT_DLL xiiJoltCollisionMeshContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltCollisionMeshContext, xiiEngineProcessDocumentContext);

public:
  xiiJoltCollisionMeshContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

  const xiiJoltMeshResourceHandle& GetMesh() const { return m_hMesh; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;
  virtual bool                         UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg);

  xiiGameObject*            m_pMeshObject;
  xiiJoltMeshResourceHandle m_hMesh;
};
