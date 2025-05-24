#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

class xiiObjectSelectionMsgToEngine;

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
  xiiMeshResourceHandle     m_hSphereMesh;
  xiiMeshResourceHandle     m_hBoxMesh;
  xiiMeshResourceHandle     m_hPlaneMesh;
  xiiComponentHandle        m_hMeshComponent;

  enum class PreviewModel : xiiUInt8
  {
    Sphere = 0,
    Box,
    Plane,
  };

  PreviewModel m_PreviewModel = PreviewModel::Sphere;
};
