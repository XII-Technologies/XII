#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/EditTools/EditTool.h>

struct xiiEngineWindowEvent;
struct xiiGameObjectEvent;
struct xiiDocumentObjectStructureEvent;
struct xiiManipulatorManagerEvent;
struct xiiSelectionManagerEvent;
struct xiiCommandHistoryEvent;
struct xiiGizmoEvent;

class XII_EDITORFRAMEWORK_DLL xiiGameObjectGizmoEditTool : public xiiGameObjectEditTool
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameObjectGizmoEditTool, xiiGameObjectEditTool);

public:
  xiiGameObjectGizmoEditTool();
  ~xiiGameObjectGizmoEditTool();

  void TransformationGizmoEventHandler(const xiiGizmoEvent& e);

protected:
  virtual void OnConfigured() override;

  void UpdateGizmoSelectionList();

  void         UpdateGizmoVisibleState();
  virtual void ApplyGizmoVisibleState(bool visible) = 0;

  void         UpdateGizmoTransformation();
  virtual void ApplyGizmoTransformation(const xiiTransform& transform) = 0;

  virtual void TransformationGizmoEventHandlerImpl(const xiiGizmoEvent& e) = 0;

  xiiDeque<xiiSelectedGameObject> m_GizmoSelection;
  bool                            m_bInGizmoInteraction = false;
  bool                            m_bMergeTransactions  = false;

private:
  void DocumentWindowEventHandler(const xiiQtDocumentWindowEvent& e);
  void UpdateManipulatorVisibility();
  void GameObjectEventHandler(const xiiGameObjectEvent& e);
  void CommandHistoryEventHandler(const xiiCommandHistoryEvent& e);
  void SelectionManagerEventHandler(const xiiSelectionManagerEvent& e);
  void ManipulatorManagerEventHandler(const xiiManipulatorManagerEvent& e);
  void EngineWindowEventHandler(const xiiEngineWindowEvent& e);
  void ObjectStructureEventHandler(const xiiDocumentObjectStructureEvent& e);
};
