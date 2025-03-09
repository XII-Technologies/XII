#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/EditTools/GizmoEditTool.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>

class xiiQtGameObjectDocumentWindow;
class xiiPreferences;

class XII_EDITORFRAMEWORK_DLL xiiTranslateGizmoEditTool : public xiiGameObjectGizmoEditTool
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTranslateGizmoEditTool, xiiGameObjectGizmoEditTool);

public:
  xiiTranslateGizmoEditTool();
  ~xiiTranslateGizmoEditTool();

  virtual xiiEditToolSupportedSpaces GetSupportedSpaces() const override { return xiiEditToolSupportedSpaces::LocalAndWorldSpace; }
  virtual bool                       GetSupportsMoveParentOnly() const override { return true; }
  virtual void                       GetGridSettings(xiiGridSettingsMsgToEngine& out_gridSettings) override;

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const xiiTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const xiiGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  void OnPreferenceChange(xiiPreferences* pref);

  xiiTranslateGizmo m_TranslateGizmo;
  enum GridPlane
  {
    X,
    Y,
    Z
  };

  GridPlane m_GridPlane = GridPlane::Z;
};

//////////////////////////////////////////////////////////////////////////

class XII_EDITORFRAMEWORK_DLL xiiRotateGizmoEditTool : public xiiGameObjectGizmoEditTool
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRotateGizmoEditTool, xiiGameObjectGizmoEditTool);

public:
  xiiRotateGizmoEditTool();
  ~xiiRotateGizmoEditTool();

  virtual xiiEditToolSupportedSpaces GetSupportedSpaces() const override { return xiiEditToolSupportedSpaces::LocalAndWorldSpace; }
  virtual bool                       GetSupportsMoveParentOnly() const override { return true; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const xiiTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const xiiGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  xiiRotateGizmo m_RotateGizmo;
};

//////////////////////////////////////////////////////////////////////////

class XII_EDITORFRAMEWORK_DLL xiiScaleGizmoEditTool : public xiiGameObjectGizmoEditTool
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScaleGizmoEditTool, xiiGameObjectGizmoEditTool);

public:
  xiiScaleGizmoEditTool();
  ~xiiScaleGizmoEditTool();

  virtual xiiEditToolSupportedSpaces GetSupportedSpaces() const override { return xiiEditToolSupportedSpaces::LocalSpaceOnly; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const xiiTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const xiiGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  xiiScaleGizmo m_ScaleGizmo;
};

//////////////////////////////////////////////////////////////////////////

class XII_EDITORFRAMEWORK_DLL xiiDragToPositionGizmoEditTool : public xiiGameObjectGizmoEditTool
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDragToPositionGizmoEditTool, xiiGameObjectGizmoEditTool);

public:
  xiiDragToPositionGizmoEditTool();
  ~xiiDragToPositionGizmoEditTool();

  virtual xiiEditToolSupportedSpaces GetSupportedSpaces() const override { return xiiEditToolSupportedSpaces::LocalSpaceOnly; }
  virtual bool                       GetSupportsMoveParentOnly() const override { return true; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const xiiTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const xiiGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  xiiDragToPositionGizmo m_DragToPosGizmo;
};
