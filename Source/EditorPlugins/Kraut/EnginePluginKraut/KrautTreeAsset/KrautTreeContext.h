#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginKraut/EnginePluginKrautDLL.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <RendererCore/Meshes/MeshResource.h>

class xiiObjectSelectionMsgToEngine;
class xiiRenderContext;

class XII_ENGINEPLUGINKRAUT_DLL xiiKrautTreeContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiKrautTreeContext, xiiEngineProcessDocumentContext);

public:
  xiiKrautTreeContext();

  virtual void                           HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;
  const xiiKrautGeneratorResourceHandle& GetResource() const { return m_hMainResource; }

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;
  virtual bool                         UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg);

  xiiGameObject*                  m_pMainObject;
  xiiComponentHandle              m_hKrautComponent;
  xiiKrautGeneratorResourceHandle m_hMainResource;
  xiiMeshResourceHandle           m_hPreviewMeshResource;
  xiiUInt32                       m_uiDisplayRandomSeed = 0xFFFFFFFF;
};
