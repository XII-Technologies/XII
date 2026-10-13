/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Gizmos/ClickGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ToolsFoundation/Animation/ExposedBone.h>

struct xiiGizmoEvent;
struct xiiAssetCuratorEvent;

/// Edits native named bone transforms using skeleton asset metadata for the hierarchy.
///
/// Enabled by attaching the xiiBoneManipulatorAttribute.
class XII_EDITORFRAMEWORK_DLL xiiBoneManipulatorAdapter : public xiiManipulatorAdapter
{
public:
  xiiBoneManipulatorAdapter();
  ~xiiBoneManipulatorAdapter();

protected:
  virtual void Finalize() override;

  virtual void Update() override;
  void         RotateGizmoEventHandler(const xiiGizmoEvent& e);
  void         ClickGizmoEventHandler(const xiiGizmoEvent& e);
  void         AssetCuratorEventHandler(const xiiAssetCuratorEvent& e);

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

  xiiDynamicArray<xiiMat4>                            m_ParentTransforms;
  xiiUuid                                             m_SkeletonAssetGuid;
  bool                                                m_bSkeletonMetadataDirty = false;
  xiiEvent<const xiiAssetCuratorEvent&>::Unsubscriber m_AssetEventsUnsubscriber;

  xiiResult RetrieveBones();
  void      ConfigureGizmos();
  void      SetTransform(xiiUInt32 uiBone, const xiiTransform& value);

  static xiiString s_sLastSelectedBone;
};
