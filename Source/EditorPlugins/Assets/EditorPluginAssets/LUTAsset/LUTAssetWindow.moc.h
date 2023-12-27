#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiLUTAssetDocument;

class xiiQtLUTAssetDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtLUTAssetDocumentWindow(xiiLUTAssetDocument* pDocument);

  virtual xiiStringView GetWindowLayoutGroupName() const override { return "LUTAsset"; }
};

class xiiLUTAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(xiiStringView sMapping);
};
