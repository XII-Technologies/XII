#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorPluginParticle/Actions/ParticleActions.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

void OnLoadPlugin()
{
  xiiParticleActions::RegisterActions();

  // Particle Effect
  {
    // Menu Bar
    {
      xiiActionMapManager::RegisterActionMap("ParticleEffectAssetMenuBar").IgnoreResult();
      xiiStandardMenus::MapActions("ParticleEffectAssetMenuBar", xiiStandardMenuTypes::File | xiiStandardMenuTypes::Edit | xiiStandardMenuTypes::Panels | xiiStandardMenuTypes::Help);
      xiiProjectActions::MapActions("ParticleEffectAssetMenuBar");
      xiiDocumentActions::MapActions("ParticleEffectAssetMenuBar", "Menu.File", false);
      xiiCommandHistoryActions::MapActions("ParticleEffectAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      xiiActionMapManager::RegisterActionMap("ParticleEffectAssetToolBar").IgnoreResult();
      xiiDocumentActions::MapActions("ParticleEffectAssetToolBar", "", true);
      xiiCommandHistoryActions::MapActions("ParticleEffectAssetToolBar", "");
      xiiAssetActions::MapActions("ParticleEffectAssetToolBar", true);
      xiiParticleActions::MapActions("ParticleEffectAssetToolBar", "");
    }

    // View Tool Bar
    {
      xiiActionMapManager::RegisterActionMap("ParticleEffectAssetViewToolBar").IgnoreResult();
      xiiViewActions::MapActions("ParticleEffectAssetViewToolBar", "", xiiViewActions::RenderMode | xiiViewActions::ActivateRemoteProcess);
      xiiViewLightActions::MapActions("ParticleEffectAssetViewToolBar", "");
    }

    xiiPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(xiiParticleEffectAssetDocument::PropertyMetaStateEventHandler);
  }
}

void OnUnloadPlugin()
{
  xiiParticleActions::UnregisterActions();
  xiiPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(xiiParticleEffectAssetDocument::PropertyMetaStateEventHandler);
}

XII_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

XII_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
