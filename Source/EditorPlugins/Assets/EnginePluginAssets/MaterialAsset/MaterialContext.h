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

class XII_ENGINEPLUGINASSETS_DLL xiiMaterialContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMaterialContext, xiiEngineProcessDocumentContext);

public:
  xiiMaterialContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;
  virtual bool                         UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext) override;

private:
  xiiMaterialResourceHandle m_hMaterial;
  xiiMeshResourceHandle     m_hBallMesh;
  xiiMeshResourceHandle     m_hSphereMesh;
  xiiMeshResourceHandle     m_hBoxMesh;
  xiiMeshResourceHandle     m_hPlaneMesh;
  xiiComponentHandle        m_hMeshComponent;

  enum class PreviewModel : xiiUInt8
  {
    Ball,
    Sphere,
    Box,
    Plane,
  };

  PreviewModel m_PreviewModel = PreviewModel::Ball;
};
