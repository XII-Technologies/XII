#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <RendererCore/Meshes/MeshComponent.h>

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiGizmoRenderData : public xiiMeshRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGizmoRenderData, xiiMeshRenderData);

public:
  xiiColor m_GizmoColor;
  bool     m_bUseDepthPrepass;
  bool     m_bIsPickable;
};

class xiiGizmoComponent;
class xiiGizmoComponentManager : public xiiComponentManager<xiiGizmoComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiGizmoComponentManager(xiiWorld* pWorld);

  xiiUInt32 m_uiHighlightID = 0;
};

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiGizmoComponent : public xiiMeshComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiGizmoComponent, xiiMeshComponent, xiiGizmoComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiMeshComponentBase

protected:
  virtual xiiMeshRenderData* CreateRenderData() const override;
  virtual xiiResult          GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiGizmoComponent

public:
  xiiGizmoComponent();
  ~xiiGizmoComponent();

  xiiColor m_GizmoColor;
  bool     m_bUseDepthPrepass = false;
  bool     m_bIsPickable      = true;
};
