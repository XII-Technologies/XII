/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <GraphicsCore/Meshes/MeshComponent.h>

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiGizmoRenderData : public xiiMeshRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGizmoRenderData, xiiMeshRenderData);

public:
  xiiColor m_GizmoColor;
  bool     m_bIsPickable;
};

class xiiGizmoComponent;
class xiiGizmoComponentManager : public xiiComponentManager<xiiGizmoComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiGizmoComponentManager(xiiWorld* pWorld);

  xiiUInt32 m_uiHighlightID = 0;
};

/// \brief Used by the editor to render gizmo meshes.
///
/// Gizmos use special shaders to have constant screen-space size and swap geometry towards the viewer,
/// so their culling is non-trivial. This component takes care of that and of the highlight color.
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

  xiiColor m_GizmoColor  = xiiColor::White;
  bool     m_bIsPickable = true;
};
