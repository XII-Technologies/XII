#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

class xiiObjectSelectionMsgToEngine;
class xiiRenderContext;

class XII_ENGINEPLUGINASSETS_DLL xiiTextureContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureContext, xiiEngineProcessDocumentContext);

public:
  xiiTextureContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

  const xiiTexture2DResourceHandle& GetTexture() const { return m_hTexture; }
  int                               GetLodLevel() const { return m_iLodLevel; }

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;

private:
  void SetTexture(xiiStringView sTextureFile);
  void OnResourceEvent(const xiiResourceEvent& e);

  xiiGameObjectHandle        m_hPreviewObject;
  xiiComponentHandle         m_hPreviewMesh2D;
  xiiMeshResourceHandle      m_hPreviewMeshResource;
  xiiMaterialResourceHandle  m_hMaterial;
  xiiTexture2DResourceHandle m_hTexture;

  xiiEvent<const xiiResourceEvent&, xiiMutex>::Unsubscriber m_TextureResourceEventSubscriber;

  int m_iLodLevel = -1;
};
