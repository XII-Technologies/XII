#pragma once

#include <Core/World/GameObject.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/Math/Mat4.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class xiiWorld;
class xiiGizmoComponent;
class xiiGizmo;

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiGizmoHandle : public xiiEditorEngineSyncObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGizmoHandle, xiiEditorEngineSyncObject);

public:
  xiiGizmoHandle();

  xiiGizmo* GetOwnerGizmo() const { return m_pParentGizmo; }

  void SetVisible(bool bVisible);

  void SetTransformation(const xiiTransform& m);
  void SetTransformation(const xiiMat4& m);

  const xiiTransform& GetTransformation() const { return m_Transformation; }

protected:
  bool         m_bVisible = false;
  xiiTransform m_Transformation;

  void SetParentGizmo(xiiGizmo* pParentGizmo) { m_pParentGizmo = pParentGizmo; }

private:
  xiiGizmo* m_pParentGizmo = nullptr;
};


enum xiiEngineGizmoHandleType
{
  Arrow,
  Ring,
  Rect,
  LineRect,
  Box,
  Piston,
  HalfPiston,
  Sphere,
  CylinderZ,
  HalfSphereZ,
  BoxCorners,
  BoxEdges,
  BoxFaces,
  LineBox,
  Cone,
  Frustum,
  FromFile,
};

struct xiiGizmoFlags
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Default = 0,

    ConstantSize = XII_BIT(0),
    OnTop        = XII_BIT(1),
    Visualizer   = XII_BIT(2),
    ShowInOrtho  = XII_BIT(3),
    Pickable     = XII_BIT(4),
    FaceCamera   = XII_BIT(5),
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

class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiEngineGizmoHandle : public xiiGizmoHandle
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEngineGizmoHandle, xiiGizmoHandle);

public:
  xiiEngineGizmoHandle();
  ~xiiEngineGizmoHandle();

  void ConfigureHandle(xiiGizmo* pParentGizmo, xiiEngineGizmoHandleType type, const xiiColor& col, xiiBitflags<xiiGizmoFlags> flags, const char* szCustomMesh = nullptr);

  virtual bool SetupForEngine(xiiWorld* pWorld, xiiUInt32 uiNextComponentPickingID) override;
  virtual void UpdateForEngine(xiiWorld* pWorld) override;

  void SetColor(const xiiColor& col);

protected:
  bool                m_bConstantSize = true;
  bool                m_bAlwaysOnTop  = false;
  bool                m_bVisualizer   = false;
  bool                m_bShowInOrtho  = false;
  bool                m_bIsPickable   = true;
  bool                m_bFaceCamera   = false;
  xiiInt32            m_iHandleType   = -1;
  xiiString           m_sGizmoHandleMesh;
  xiiGameObjectHandle m_hGameObject;
  xiiGizmoComponent*  m_pGizmoComponent = nullptr;
  xiiColor            m_Color           = xiiColor::CornflowerBlue; /* The Original! */
  xiiWorld*           m_pWorld          = nullptr;
};
