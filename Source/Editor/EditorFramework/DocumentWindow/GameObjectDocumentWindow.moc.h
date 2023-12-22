#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>

class xiiGameObjectDocument;
class xiiWorldSettingsMsgToEngine;
class xiiQtGameObjectViewWidget;
struct xiiGameObjectEvent;
struct xiiSnapProviderEvent;

class XII_EDITORFRAMEWORK_DLL xiiQtGameObjectDocumentWindow : public xiiQtEngineDocumentWindow
{
  Q_OBJECT
public:
  xiiQtGameObjectDocumentWindow(xiiGameObjectDocument* pDocument);
  ~xiiQtGameObjectDocumentWindow();

  xiiGameObjectDocument* GetGameObjectDocument() const;

protected:
  xiiWorldSettingsMsgToEngine GetWorldSettings() const;
  xiiGridSettingsMsgToEngine  GetGridSettings() const;
  virtual void                ProcessMessageEventHandler(const xiiEditorEngineDocumentMsg* pMsg) override;

private:
  void GameObjectEventHandler(const xiiGameObjectEvent& e);
  void SnapProviderEventHandler(const xiiSnapProviderEvent& e);

  void FocusOnSelectionAllViews();
  void FocusOnSelectionHoveredView();

  void HandleFocusOnSelection(const xiiQuerySelectionBBoxResultMsgToEditor* pMsg, xiiQtGameObjectViewWidget* pSceneView);
};
