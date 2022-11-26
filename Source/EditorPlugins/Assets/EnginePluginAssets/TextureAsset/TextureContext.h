#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

class xiiObjectSelectionMsgToEngine;
class xiiRenderContext;

class XII_ENGINEPLUGINASSETS_DLL xiiTextureContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureContext, xiiEngineProcessDocumentContext);

public:
  xiiTextureContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

  const xiiTexture2DResourceHandle& GetTexture() const { return m_hTexture; }

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;

private:
  void OnResourceEvent(const xiiResourceEvent& e);

  xiiGameObjectHandle        m_hPreviewObject;
  xiiComponentHandle         m_hPreviewMesh2D;
  xiiMeshResourceHandle      m_hPreviewMeshResource;
  xiiMaterialResourceHandle  m_hMaterial;
  xiiTexture2DResourceHandle m_hTexture;

  xiiEvent<const xiiResourceEvent&, xiiMutex>::Unsubscriber m_TextureResourceEventSubscriber;
};
