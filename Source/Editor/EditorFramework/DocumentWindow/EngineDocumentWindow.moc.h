/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/IPC/IPCObjectMirrorEditor.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>

class QWidget;
class QHBoxLayout;
class QPushButton;
class xiiQtEngineViewWidget;
class xiiAssetDocument;
class xiiEditorEngineDocumentMsg;
struct xiiObjectPickingResult;
struct xiiEngineViewConfig;
struct xiiCommonAssetUiState;

struct XII_EDITORFRAMEWORK_DLL xiiEngineWindowEvent
{
  enum class Type
  {
    ViewCreated,
    ViewDestroyed,
  };

  Type                   m_Type;
  xiiQtEngineViewWidget* m_pView = nullptr;
};

/// Base class for all document windows that need a connection to the engine process, and might want to render 3D content.
///
/// This class has a xiiEditorEngineConnection object for sending messages between the editor and the engine process.
/// It also allows to embed xiiQtEngineViewWidget objects into the UI, which enable 3D rendering by the engine process.
class XII_EDITORFRAMEWORK_DLL xiiQtEngineDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtEngineDocumentWindow(xiiAssetDocument* pDocument);
  virtual ~xiiQtEngineDocumentWindow();

  xiiEditorEngineConnection*    GetEditorEngineConnection() const;
  const xiiObjectPickingResult& PickObject(xiiUInt16 uiScreenPosX, xiiUInt16 uiScreenPosY, xiiQtEngineViewWidget* pView) const;

  xiiAssetDocument* GetDocument() const;

  /// Returns the xiiQtEngineViewWidget over which the mouse currently hovers
  xiiQtEngineViewWidget* GetHoveredViewWidget() const;

  /// Returns the xiiQtEngineViewWidget that has the input focus
  xiiQtEngineViewWidget* GetFocusedViewWidget() const;

  xiiQtEngineViewWidget* GetViewWidgetByID(xiiUInt32 uiViewID) const;

  xiiArrayPtr<xiiQtEngineViewWidget* const> GetViewWidgets() const;

  void AddViewWidget(xiiQtEngineViewWidget* pView);

  virtual void CreateImageCapture(xiiStringView sOutputPath) override;

public:
  mutable xiiEvent<const xiiEngineWindowEvent&> m_EngineWindowEvent;

protected:
  friend class xiiQtEngineViewWidget;

  xiiHybridArray<xiiQtEngineViewWidget*, 4> m_ViewWidgets;

  virtual void CommonAssetUiEventHandler(const xiiCommonAssetUiState& e);

  virtual void ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg);
  void         RemoveViewWidget(xiiQtEngineViewWidget* pView);
  void         DestroyAllViews();
  virtual void InternalRedraw() override;
};
