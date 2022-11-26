#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>

class xiiObjectSelectionMsgToEngine;
class xiiRenderContext;

class XII_ENGINEPLUGINASSETS_DLL xiiTextureCubeContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureCubeContext, xiiEngineProcessDocumentContext);

public:
  xiiTextureCubeContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

  const xiiTextureCubeResourceHandle& GetTexture() const { return m_hTexture; }

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;

private:
  void OnResourceEvent(const xiiResourceEvent& e);

  xiiGameObjectHandle          m_hPreviewObject;
  xiiComponentHandle           m_hPreviewMesh2D;
  xiiMeshResourceHandle        m_hPreviewMeshResource;
  xiiMaterialResourceHandle    m_hMaterial;
  xiiTextureCubeResourceHandle m_hTexture;

  xiiEvent<const xiiResourceEvent&, xiiMutex>::Unsubscriber m_TextureResourceEventSubscriber;
};
