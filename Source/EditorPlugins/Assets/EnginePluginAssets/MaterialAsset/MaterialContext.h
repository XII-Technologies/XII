/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/Rendering/ScenePreviewLighting.h>
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

  const xiiPreviewLightingSettings& GetPreviewLightingSettings() const { return m_LightingSettings; }
  bool                              GetPreviewGrid() const { return m_bPreviewGrid; }

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual xiiStatus OnInitialize() override;
  virtual void      OnDeinitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;
  virtual bool                         UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext) override;

private:
  xiiUniquePtr<xiiScenePreviewLighting> m_pPreviewLighting;
  xiiPreviewLightingSettings            m_LightingSettings;
  xiiVariant                            m_LastLightingMessage;
  bool                                  m_bPreviewGrid        = false;
  float                                 m_fPreviewOrientation = 0.0f;
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
