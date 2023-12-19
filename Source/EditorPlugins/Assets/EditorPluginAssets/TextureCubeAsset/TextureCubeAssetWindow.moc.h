#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtOrbitCamViewWidget;
class xiiTextureCubeAssetDocument;

class xiiQtTextureCubeAssetDocumentWindow : public xiiQtEngineDocumentWindow
{
  Q_OBJECT

public:
  xiiQtTextureCubeAssetDocumentWindow(xiiTextureCubeAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "TextureCubeAsset"; }

private:
  virtual void InternalRedraw() override;
  void         SendRedrawMsg();

  xiiEngineViewConfig      m_ViewConfig;
  xiiQtOrbitCamViewWidget* m_pViewWidget;
};
