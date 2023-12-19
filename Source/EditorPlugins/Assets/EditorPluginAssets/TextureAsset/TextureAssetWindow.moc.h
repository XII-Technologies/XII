#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiQtOrbitCamViewWidget;
class xiiTextureAssetDocument;

class xiiQtTextureAssetDocumentWindow : public xiiQtEngineDocumentWindow
{
  Q_OBJECT

public:
  xiiQtTextureAssetDocumentWindow(xiiTextureAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "TextureAsset"; }

private:
  virtual void InternalRedraw() override;
  void         SendRedrawMsg();

  xiiEngineViewConfig      m_ViewConfig;
  xiiQtOrbitCamViewWidget* m_pViewWidget;
};

class xiiTextureChannelModeAction : public xiiEnumerationMenuAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureChannelModeAction, xiiEnumerationMenuAction);

public:
  xiiTextureChannelModeAction(const xiiActionContext& context, const char* szName, const char* szIconPath);
  virtual xiiInt64 GetValue() const override;
  virtual void     Execute(const xiiVariant& value) override;
};

class xiiTextureLodSliderAction : public xiiSliderAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureLodSliderAction, xiiSliderAction);

public:
  xiiTextureLodSliderAction(const xiiActionContext& context, const char* szName);

  virtual void Execute(const xiiVariant& value) override;

private:
  xiiTextureAssetDocument* m_pDocument;
};

class xiiTextureAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(xiiStringView sMapping);

  static xiiActionDescriptorHandle s_hTextureChannelMode;
  static xiiActionDescriptorHandle s_hLodSlider;
};
