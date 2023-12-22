#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <GraphicsCore/Meshes/MeshResource.h>

class XII_ENGINEPLUGINASSETS_DLL xiiDecalContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDecalContext, xiiEngineProcessDocumentContext);

public:
  xiiDecalContext();

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;

private:
  xiiMeshResourceHandle m_hPreviewMeshResource;

  // xiiDecalResourceHandle m_hDecal;
};
