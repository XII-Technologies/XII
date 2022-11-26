#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtEngineViewWidget;

class xiiQtRmlUiAssetDocumentWindow : public xiiQtEngineDocumentWindow
{
  Q_OBJECT

public:
  xiiQtRmlUiAssetDocumentWindow(xiiAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "RmlUiAsset"; }

protected:
  virtual void InternalRedraw() override;

private:
  void SendRedrawMsg();

  xiiEngineViewConfig    m_ViewConfig;
  xiiQtEngineViewWidget* m_pViewWidget;
  xiiRmlUiAssetDocument* m_pAssetDoc;
};
