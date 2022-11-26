#pragma once

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/PropertyGrid/Declarations.h>

struct xiiEngineViewPreferences;
class QGridLayout;
class xiiQtViewWidgetContainer;
class xiiQtSceneViewWidget;
class QSettings;
struct xiiManipulatorManagerEvent;
class xiiPreferences;
class xiiQtQuadViewWidget;
struct xiiEngineWindowEvent;
class xiiSceneDocument;
class QMenu;

Q_DECLARE_OPAQUE_POINTER(xiiQtSceneViewWidget*);

class xiiQtSceneDocumentWindowBase : public xiiQtGameObjectDocumentWindow, public xiiGameObjectGizmoInterface
{
  Q_OBJECT

public:
  xiiQtSceneDocumentWindowBase(xiiSceneDocument* pDocument);
  ~xiiQtSceneDocumentWindowBase();

  xiiSceneDocument* GetSceneDocument() const;

  virtual void CreateImageCapture(const char* szOutputPath) override;

public Q_SLOTS:
  void ToggleViews(QWidget* pView);

public:
  /// \name xiiGameObjectGizmoInterface implementation
  ///@{
  virtual xiiObjectAccessorBase* GetObjectAccessor() override;
  virtual bool                   CanDuplicateSelection() const override;
  virtual void                   DuplicateSelection() override;
  ///@}

protected:
  virtual void ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg) override;
  virtual void InternalRedraw() override;

  void GameObjectEventHandler(const xiiGameObjectEvent& e);
  void SnapSelectionToPosition(bool bSnapEachObject);
  void SendRedrawMsg();
  void ExtendPropertyGridContextMenu(QMenu& menu, const xiiHybridArray<xiiPropertySelection, 8>& items, const xiiAbstractProperty* pProp);

protected:
  xiiQtQuadViewWidget* m_pQuadViewWidget = nullptr;
};

class xiiQtSceneDocumentWindow : public xiiQtSceneDocumentWindowBase
{
  Q_OBJECT

public:
  xiiQtSceneDocumentWindow(xiiSceneDocument* pDocument);
  ~xiiQtSceneDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "Scene"; }
};
