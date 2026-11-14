/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <EditorPluginJolt/EditorPluginJoltDLL.h>

#include <EditorFramework/Gizmos/BoxGizmo.h>
#include <EditorFramework/Gizmos/CapsuleGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/SphereGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <EnginePluginJolt/JoltAuthoring.h>

class xiiJoltShapeManipulatorAdapter final : public xiiManipulatorAdapter
{
public:
  ~xiiJoltShapeManipulatorAdapter();

private:
  void              Finalize() override;
  void              Update() override;
  void              UpdateGizmoTransform() override;
  void              GizmoEvent(const xiiGizmoEvent& event);
  xiiTranslateGizmo m_Translate;
  xiiRotateGizmo    m_Rotate;
  xiiBoxGizmo       m_Box;
  xiiSphereGizmo    m_Sphere;
  xiiCapsuleGizmo   m_Capsule;
  xiiVec3           m_vCylinderSize = xiiVec3(1.0f);
  bool              m_bInteracting  = false;
};

class xiiJoltJointManipulatorAdapter final : public xiiManipulatorAdapter
{
public:
  ~xiiJoltJointManipulatorAdapter();

private:
  void              Finalize() override;
  void              Update() override;
  void              UpdateGizmoTransform() override;
  void              GizmoEvent(const xiiGizmoEvent& event);
  bool              GetActorTransform(xiiTransform& out_transform) const;
  xiiTranslateGizmo m_Translate;
  xiiRotateGizmo    m_Rotate;
  bool              m_bInteracting = false;
};

class xiiJoltVisualizerAdapter final : public xiiVisualizerAdapter
{
  void                       Finalize() override;
  void                       Update() override;
  void                       UpdateGizmoTransform() override;
  xiiJoltAuthoringSyncObject m_Sync;
};

void xiiRegisterJoltComponentAdapters();
void xiiUnregisterJoltComponentAdapters();
