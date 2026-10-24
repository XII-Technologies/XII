/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoGeometry.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Types/UniquePtr.h>
#include <Shaders/Editor/EditorGizmoData.h>

class xiiGizmo;
class xiiView;

enum class xiiGizmoVisualState : xiiInt32
{
  Normal,
  Selected,
  Active,
  Disabled,
};

struct xiiGizmoFlags
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    ConstantSize = XII_BIT(0),
    OnTop        = XII_BIT(1),
    Visualizer   = XII_BIT(2),
    ShowInOrtho  = XII_BIT(3),
    Pickable     = XII_BIT(4),
    FaceCamera   = XII_BIT(5),
    Default      = 0,
  };
  struct Bits
  {
    StorageType ConstantSize : 1;
    StorageType OnTop : 1;
    StorageType Visualizer : 1;
    StorageType ShowInOrtho : 1;
    StorageType Pickable : 1;
    StorageType FaceCamera : 1;
  };
};
XII_DECLARE_FLAGS_OPERATORS(xiiGizmoFlags);

/// Editor-side interaction identity and synchronized spatial settings. The owner is a borrowed Editor pointer
/// and never crosses IPC. Handles are owned by their Editor document or engine document sync-object collection.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiGizmoHandle : public xiiEditorEngineSyncObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGizmoHandle, xiiEditorEngineSyncObject);

public:
  void                SetVisible(bool bVisible);
  bool                IsVisible() const { return m_bVisible; }
  void                SetTransformation(const xiiTransform& transform);
  void                SetTransformation(const xiiMat4& transform);
  const xiiTransform& GetTransformation() const { return m_Transformation; }
  xiiGizmo*           GetOwnerGizmo() const { return m_pOwnerGizmo; }

protected:
  xiiGizmo*    m_pOwnerGizmo    = nullptr;
  bool         m_bVisible       = false;
  xiiTransform m_Transformation = xiiTransform::MakeIdentity();
};

/// A synchronized Editor handle. Compact view-local descriptions feed the dedicated gizmo
/// operation; immediate CPU picking uses matching primitive definitions and view transforms.
/// Access and global scale changes require exclusive Editor/engine-process access, as with other sync objects.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEngineGizmoHandle : public xiiGizmoHandle
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEngineGizmoHandle, xiiGizmoHandle);

public:
  xiiEngineGizmoHandle();
  ~xiiEngineGizmoHandle();

  void ConfigureHandle(xiiGizmo* pOwner, xiiEngineGizmoHandleType type, const xiiColor& color, xiiBitflags<xiiGizmoFlags> flags, const char* szCustomMesh = nullptr);
  void SetColor(const xiiColor& color);
  void SetVisualState(xiiGizmoVisualState state);
  void SetVisualParameters(float fArcStart, float fArcDelta);
  void SetDragOrigin(const xiiVec3& vOrigin);
  void SetInteractionText(xiiStringView text);
  bool IsAlwaysOnTop() const { return m_bAlwaysOnTop; }
  /// Matches the draw layers. The uniform-scale target stays accessible where end-on axes overlap it.
  xiiUInt32 GetPickingPriority() const { return !m_bAlwaysOnTop ? 0U : m_iHandleType == static_cast<xiiInt32>(xiiEngineGizmoHandleType::ScaleUniform) ? 2U :
                                                                                                                                                        1U; }
  xiiUInt32 GetPickingID() const { return m_uiPickingID; }

  /// Reserves an identity once, including for initially non-pickable handles whose flags can change via IPC.
  bool SetupForEngine(xiiWorld* pWorld, xiiUInt32 uiNextComponentPickingID) override;
  /// Invalidates CPU geometry after an IPC update. No world access or allocation is needed here.
  void UpdateForEngine(xiiWorld* pWorld) override;

  /// Positive finite user preference; one local unit occupies 128 logical pixels when ConstantSize is enabled.
  static xiiResult SetGizmoScale(float fScale);
  static float     GetGizmoScale();

  /// Fails for hidden handles, excluded orthographic handles, invalid views/transforms, or a perspective
  /// center behind the camera. Outputs are unchanged on failure. Camera-facing planes flip local axes
  /// toward the camera, preserving the manipulator's axes instead of rotating them into a billboard.
  xiiResult ComputeViewTransform(const xiiView& view, xiiMat4& out_transform, float& out_fWorldUnitsPerPixel) const;
  /// Hidden/excluded handles emit nothing. Color/transform/state updates never rebuild render geometry.
  xiiResult AppendRenderData(const xiiView& view, xiiUInt32 uiHighlightID, xiiDynamicArray<xiiEditorGizmoInstance>& instances, xiiVariantArray& customMeshes) const;
  /// Misses and non-pickable visualizers return failure without changing out_fRayTime.
  /// Lines have a four-pixel world-space tolerance. No allocation after geometry has been built.
  xiiResult PickRay(const xiiView& view, const xiiVec3& vStart, const xiiVec3& vDirection, float& out_fRayTime);

private:
  xiiResult EnsureGeometry();

  xiiInt32  m_iHandleType  = -1;
  xiiInt32  m_iVisualState = 0;
  xiiVec2   m_vArc         = xiiVec2::MakeZero();
  xiiVec3   m_vDragOrigin  = xiiVec3::MakeZero();
  xiiString m_sGizmoHandleMesh;
  xiiString m_sInteractionText;
  xiiColor  m_Color         = xiiColor::CornflowerBlue;
  bool      m_bConstantSize = false;
  bool      m_bAlwaysOnTop  = false;
  bool      m_bVisualizer   = false;
  bool      m_bShowInOrtho  = false;
  bool      m_bIsPickable   = false;
  bool      m_bFaceCamera   = false;

  xiiUInt32                      m_uiPickingID = 0;
  xiiUniquePtr<xiiGizmoGeometry> m_pGeometry;
  bool                           m_bGeometryDirty   = true;
  xiiResult                      m_GeometryResult   = XII_FAILURE;
  xiiInt32                       m_iBuiltHandleType = -1;
  xiiString                      m_sBuiltMesh;
};
