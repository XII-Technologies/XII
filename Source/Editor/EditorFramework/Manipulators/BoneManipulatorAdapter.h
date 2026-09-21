/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Gizmos/ClickGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <Foundation/Containers/DynamicArray.h>
#include <GraphicsCore/AnimationSystem/EditableSkeleton.h>

struct xiiGizmoEvent;

/// Makes an array of xiiExposedBone properties editable in the viewport
///
/// Enabled by attaching the xiiBoneManipulatorAttribute.
class xiiBoneManipulatorAdapter : public xiiManipulatorAdapter
{
public:
  xiiBoneManipulatorAdapter();
  ~xiiBoneManipulatorAdapter();

protected:
  virtual void Finalize() override;

  void MigrateSelection();

  virtual void Update() override;
  void         RotateGizmoEventHandler(const xiiGizmoEvent& e);
  void         ClickGizmoEventHandler(const xiiGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  struct ElementGizmo
  {
    xiiMat4        m_Offset;
    xiiMat4        m_InverseOffset;
    xiiRotateGizmo m_RotateGizmo;
    xiiClickGizmo  m_ClickGizmo;
  };

  xiiVariantArray                 m_Keys;
  xiiDynamicArray<xiiExposedBone> m_Bones;
  xiiDeque<ElementGizmo>          m_Gizmos;
  xiiTransform                    m_RootTransform = xiiTransform::MakeIdentity();

  void    RetrieveBones();
  void    ConfigureGizmos();
  void    SetTransform(xiiUInt32 uiBone, const xiiTransform& value);
  xiiMat4 ComputeFullTransform(xiiUInt32 uiBone) const;
  xiiMat4 ComputeParentTransform(xiiUInt32 uiBone) const;

  static xiiString s_sLastSelectedBone;
};
