/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <GuiFoundation/Action/BaseActions.h>

/// Actions for configuring the engine view light settings.
class XII_EDITORFRAMEWORK_DLL xiiViewLightActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(xiiStringView sMapping);

  static xiiActionDescriptorHandle s_hLightMenu;
  static xiiActionDescriptorHandle s_hSkyBox;
  static xiiActionDescriptorHandle s_hSkyLight;
  static xiiActionDescriptorHandle s_hSkyLightCubeMap;
  static xiiActionDescriptorHandle s_hSkyLightIntensity;
  static xiiActionDescriptorHandle s_hDirLight;
  static xiiActionDescriptorHandle s_hDirLightAngle;
  static xiiActionDescriptorHandle s_hDirLightShadows;
  static xiiActionDescriptorHandle s_hDirLightIntensity;
  static xiiActionDescriptorHandle s_hFog;
  static xiiActionDescriptorHandle s_hSetAsDefault;
};

class XII_EDITORFRAMEWORK_DLL xiiViewLightButtonAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewLightButtonAction, xiiButtonAction);

public:
  xiiViewLightButtonAction(const xiiActionContext& context, const char* szName, xiiEngineViewLightSettingsEvent::Type button);
  ~xiiViewLightButtonAction();

  virtual void Execute(const xiiVariant& value) override;
  void         LightSettingsEventHandler(const xiiEngineViewLightSettingsEvent& e);
  void         UpdateAction();

private:
  xiiEngineViewLightSettingsEvent::Type m_ButtonType;
  xiiEngineViewLightSettings*           m_pSettings = nullptr;
  xiiEventSubscriptionID                m_SettingsID;
};

class XII_EDITORFRAMEWORK_DLL xiiViewLightSliderAction : public xiiSliderAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiViewLightSliderAction, xiiSliderAction);

public:
  xiiViewLightSliderAction(const xiiActionContext& context, const char* szName, xiiEngineViewLightSettingsEvent::Type button);
  ~xiiViewLightSliderAction();

  virtual void Execute(const xiiVariant& value) override;
  void         LightSettingsEventHandler(const xiiEngineViewLightSettingsEvent& e);
  void         UpdateAction();

private:
  xiiEngineViewLightSettingsEvent::Type m_ButtonType;
  xiiEngineViewLightSettings*           m_pSettings = nullptr;
  xiiEventSubscriptionID                m_SettingsID;
};
