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

class xiiTextureCubeChannelModeAction : public xiiEnumerationMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureCubeChannelModeAction, xiiEnumerationMenuAction);

public:
  xiiTextureCubeChannelModeAction(const xiiActionContext& context, const char* szName, const char* szIconPath);
  virtual xiiInt64 GetValue() const override;
  virtual void     Execute(const xiiVariant& value) override;
};

class xiiTextureCubeLodSliderAction : public xiiSliderAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureCubeLodSliderAction, xiiSliderAction);

public:
  xiiTextureCubeLodSliderAction(const xiiActionContext& context, const char* szName);

  virtual void Execute(const xiiVariant& value) override;

private:
  xiiTextureCubeAssetDocument* m_pDocument;
};

class xiiTextureCubeAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static xiiActionDescriptorHandle s_hTextureChannelMode;
  static xiiActionDescriptorHandle s_hLodSlider;
};
