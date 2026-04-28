/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtColorGradientEditorWidget;

class xiiQtColorGradientAssetDocumentWindow : public xiiQtDocumentWindow
{
  Q_OBJECT

public:
  xiiQtColorGradientAssetDocumentWindow(xiiDocument* pDocument);
  ~xiiQtColorGradientAssetDocumentWindow();

  virtual xiiStringView GetWindowLayoutGroupName() const override { return "ColorGradientAsset"; }

private Q_SLOTS:
  void onGradientColorCpAdded(double posX, const xiiColorGammaUB& color);
  void onGradientAlphaCpAdded(double posX, xiiUInt8 alpha);
  void onGradientIntensityCpAdded(double posX, float intensity);

  void MoveCP(xiiInt32 idx, double newPosX, const char* szArrayName);
  void onGradientColorCpMoved(xiiInt32 idx, double newPosX);
  void onGradientAlphaCpMoved(xiiInt32 idx, double newPosX);
  void onGradientIntensityCpMoved(xiiInt32 idx, double newPosX);

  void RemoveCP(xiiInt32 idx, const char* szArrayName);
  void onGradientColorCpDeleted(xiiInt32 idx);
  void onGradientAlphaCpDeleted(xiiInt32 idx);
  void onGradientIntensityCpDeleted(xiiInt32 idx);

  void onGradientColorCpChanged(xiiInt32 idx, const xiiColorGammaUB& color);
  void onGradientAlphaCpChanged(xiiInt32 idx, xiiUInt8 alpha);
  void onGradientIntensityCpChanged(xiiInt32 idx, float intensity);

  void onGradientBeginOperation();
  void onGradientEndOperation(bool commit);

  void onGradientNormalizeRange();

private:
  void UpdatePreview();

  void SendLiveResourcePreview();
  void RestoreResource();

  void PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const xiiDocumentObjectStructureEvent& e);

  bool                            m_bShowFirstTime;
  xiiQtColorGradientEditorWidget* m_pGradientEditor;
};
