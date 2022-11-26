#pragma once

#include <EditorPluginParticle/EditorPluginParticleDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class xiiParticleEffectAssetDocument;
struct xiiParticleEffectAssetEvent;

class xiiParticleActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static xiiActionDescriptorHandle s_hCategory;
  static xiiActionDescriptorHandle s_hPauseEffect;
  static xiiActionDescriptorHandle s_hRestartEffect;
  static xiiActionDescriptorHandle s_hAutoRestart;
  static xiiActionDescriptorHandle s_hSimulationSpeedMenu;
  static xiiActionDescriptorHandle s_hSimulationSpeed[10];
  static xiiActionDescriptorHandle s_hRenderVisualizers;
};

class xiiParticleAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleAction, xiiButtonAction);

public:
  enum class ActionType
  {
    PauseEffect,
    RestartEffect,
    AutoRestart,
    SimulationSpeed,
    RenderVisualizers,
  };

  xiiParticleAction(const xiiActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);
  ~xiiParticleAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  void EffectEventHandler(const xiiParticleEffectAssetEvent& e);
  void UpdateState();

  xiiParticleEffectAssetDocument* m_pEffectDocument;
  ActionType                      m_Type;
  float                           m_fSimSpeed;
};
