#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class xiiAssetDocument;

class XII_EDITORFRAMEWORK_DLL xiiCommonAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(xiiStringView sMapping, xiiUInt32 uiStateMask);

  static xiiActionDescriptorHandle s_hCategory;
  static xiiActionDescriptorHandle s_hPause;
  static xiiActionDescriptorHandle s_hRestart;
  static xiiActionDescriptorHandle s_hLoop;
  static xiiActionDescriptorHandle s_hSimulationSpeedMenu;
  static xiiActionDescriptorHandle s_hSimulationSpeed[10];
  static xiiActionDescriptorHandle s_hGrid;
  static xiiActionDescriptorHandle s_hVisualizers;
};

class XII_EDITORFRAMEWORK_DLL xiiCommonAssetAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCommonAssetAction, xiiButtonAction);

public:
  enum class ActionType
  {
    Pause,
    Restart,
    Loop,
    SimulationSpeed,
    Grid,
    Visualizers,
  };

  xiiCommonAssetAction(const xiiActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);
  ~xiiCommonAssetAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  void CommonUiEventHandler(const xiiCommonAssetUiState& e);
  void UpdateState();

  xiiAssetDocument* m_pAssetDocument = nullptr;
  ActionType        m_Type;
  float             m_fSimSpeed;
};
