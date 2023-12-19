#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class xiiPreferences;
struct xiiGameObjectEvent;
class xiiGameObjectDocument;
///
class XII_EDITORFRAMEWORK_DLL xiiGameObjectDocumentActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(xiiStringView sMapping);
  static void MapMenuSimulationSpeed(xiiStringView sMapping);

  static void MapToolbarActions(xiiStringView sMapping);

  static xiiActionDescriptorHandle s_hGameObjectCategory;
  static xiiActionDescriptorHandle s_hRenderSelectionOverlay;
  static xiiActionDescriptorHandle s_hRenderVisualizers;
  static xiiActionDescriptorHandle s_hRenderShapeIcons;
  static xiiActionDescriptorHandle s_hRenderGrid;
  static xiiActionDescriptorHandle s_hAddAmbientLight;
  static xiiActionDescriptorHandle s_hSimulationSpeedMenu;
  static xiiActionDescriptorHandle s_hSimulationSpeed[10];
  static xiiActionDescriptorHandle s_hCameraSpeed;
  static xiiActionDescriptorHandle s_hPickTransparent;
};

///
class XII_EDITORFRAMEWORK_DLL xiiGameObjectDocumentAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGameObjectDocumentAction, xiiButtonAction);

public:
  enum class ActionType
  {
    RenderSelectionOverlay,
    RenderVisualizers,
    RenderShapeIcons,
    RenderGrid,
    AddAmbientLight,
    SimulationSpeed,
    PickTransparent,
  };

  xiiGameObjectDocumentAction(const xiiActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);
  ~xiiGameObjectDocumentAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  void SceneEventHandler(const xiiGameObjectEvent& e);
  void OnPreferenceChange(xiiPreferences* pref);

  float                  m_fSimSpeed;
  xiiGameObjectDocument* m_pGameObjectDocument;
  ActionType             m_Type;
};


class XII_EDITORFRAMEWORK_DLL xiiCameraSpeedSliderAction : public xiiSliderAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCameraSpeedSliderAction, xiiSliderAction);

public:
  enum class ActionType
  {
    CameraSpeed,
  };

  xiiCameraSpeedSliderAction(const xiiActionContext& context, const char* szName, ActionType type);
  ~xiiCameraSpeedSliderAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  void OnPreferenceChange(xiiPreferences* pref);
  void UpdateState();

  xiiGameObjectDocument* m_pGameObjectDocument;
  ActionType             m_Type;
};
