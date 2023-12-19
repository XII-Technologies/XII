#pragma once

#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/DrawBoxGizmo.h>

struct xiiGameObjectEvent;
struct xiiManipulatorManagerEvent;

class XII_EDITORPLUGINSCENE_DLL xiiGreyBoxEditTool : public xiiGameObjectEditTool
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGreyBoxEditTool, xiiGameObjectEditTool);

public:
  xiiGreyBoxEditTool();
  ~xiiGreyBoxEditTool();

  virtual xiiEditorInputContext*     GetEditorInputContextOverride() override;
  virtual xiiEditToolSupportedSpaces GetSupportedSpaces() const override;
  virtual bool                       GetSupportsMoveParentOnly() const override;
  virtual void                       GetGridSettings(xiiGridSettingsMsgToEngine& out_gridSettings) override;

protected:
  virtual void OnConfigured() override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  void UpdateGizmoState();
  void GameObjectEventHandler(const xiiGameObjectEvent& e);
  void ManipulatorManagerEventHandler(const xiiManipulatorManagerEvent& e);
  void GizmoEventHandler(const xiiGizmoEvent& e);

  xiiDrawBoxGizmo m_DrawBoxGizmo;
};
