/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Communication/Event.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

#include <QPointer>

class xiiImageDataAssetDocument;
struct xiiImageDataAssetEvent;

class xiiQtImageDataAssetDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtImageDataAssetDocumentWindow(xiiImageDataAssetDocument* pDocument);

  virtual xiiStringView GetWindowLayoutGroupName() const override { return "ImageDataAsset"; }

private:
  void                                                  ImageDataAssetEventHandler(const xiiImageDataAssetEvent& e);
  xiiEvent<const xiiImageDataAssetEvent&>::Unsubscriber m_EventUnsubscriper;

  void UpdatePreview();

  QPointer<xiiQtImageWidget> m_pImageWidget;
};
